/*
 * Paulo Pedreiras, 2022/10
 * Zephyr: Simple thread creation example (2)
 *
 * One of the tasks is periodc, the other two are activated via a semaphore. Data communicated via sharem memory
 *
 * Base documentation:
 *      https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/reference/kernel/index.html
 *
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <sys/printk.h>
#include <sys/__assert.h>
#include <string.h>
#include <timing/timing.h>
#include <stdlib.h>
#include <stdio.h>

/*ADC definitions and includes*/
#include <hal/nrf_saadc.h>
#define ADC_RESOLUTION 10
#define ADC_GAIN ADC_GAIN_1_4
#define ADC_REFERENCE ADC_REF_VDD_1_4
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40)
#define ADC_CHANNEL_ID 1

#define ADC_CHANNEL_INPUT NRF_SAADC_INPUT_AIN1

#define BUFFER_SIZE 1

#define ADC_NODE DT_NODELABEL(adc)
const struct device *adc_dev = NULL;

/* Other defines */
#define TIMER_INTERVAL_MSEC 1000 /* Interval between ADC samples */

/* ADC channel configuration */
static const struct adc_channel_cfg my_channel_cfg = {
    .gain = ADC_GAIN,
    .reference = ADC_REFERENCE,
    .acquisition_time = ADC_ACQUISITION_TIME,
    .channel_id = ADC_CHANNEL_ID,
    .input_positive = ADC_CHANNEL_INPUT};

/* Global vars */
struct k_timer my_timer;
static uint16_t adc_sample_buffer[BUFFER_SIZE];

/* Size of stack area used by each thread (can be thread specific, if necessary)*/
#define STACK_SIZE 1024

/* Thread scheduling priority */
#define thread_sensor_prio 1
#define thread_processing_prio 1
#define thread_output_prio 1

/* Therad periodicity (in ms)*/
#define SAMP_PERIOD_MS 1000

#define LED0_NODE DT_NODELABEL(led0)
#define LED1_NODE DT_NODELABEL(led1)
#define LED2_NODE DT_NODELABEL(led2)
#define LED3_NODE DT_NODELABEL(led3)

/* Create thread stack space */
K_THREAD_STACK_DEFINE(thread_sensor_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_processing_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_output_stack, STACK_SIZE);

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

/* Create variables for thread data */
struct k_thread thread_sensor_data;
struct k_thread thread_processing_data;
struct k_thread thread_output_data;

/* Create task IDs */
k_tid_t thread_sensor_tid;
k_tid_t thread_processing_tid;
k_tid_t thread_output_tid;

/* Global vars (shared memory between tasks A/B and B/C, resp) */
uint16_t ab = 100;
uint16_t bc = 200;

/* Semaphores for task synch */
struct k_sem sem_ab;
struct k_sem sem_bc;

/* Thread code prototypes */
void thread_sensor_code(void *argA, void *argB, void *argC);
void thread_processing_code(void *argA, void *argB, void *argC);
void thread_output_code(void *argA, void *argB, void *argC);
int precedentsAverage(uint16_t *precedents, uint16_t current_read, uint16_t precedents_size);


/* Takes one sample */
static int adc_sample(void)
{
    int ret;
    const struct adc_sequence sequence = {
        .channels = BIT(ADC_CHANNEL_ID),
        .buffer = adc_sample_buffer,
        .buffer_size = sizeof(adc_sample_buffer),
        .resolution = ADC_RESOLUTION,
    };

    if (adc_dev == NULL)
    {
        printk("adc_sample(): error, must bind to adc first \n\r");
        return -1;
    }

    ret = adc_read(adc_dev, &sequence);
    if (ret)
    {
        printk("adc_read() failed with code %d\n", ret);
    }

    return ret;
}


/* Main function */
void main(void)
{
    int err = 0;
    int ret;
    /* Configure the GPIO pin for output */
    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    if (ret < 0)
    {
        return;
    }
    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    if (ret < 0)
    {
        return;
    }
    ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
    if (ret < 0)
    {
        return;
    }
    ret = gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
    if (ret < 0)
    {
        return;
    }


    adc_dev = device_get_binding(DT_LABEL(ADC_NODE));
    if (!adc_dev)
    {
        printk("ADC device_get_binding() failed\n");
    }
    err = adc_channel_setup(adc_dev, &my_channel_cfg);
    if (err)
    {
        printk("adc_channel_setup() failed with error code %d\n", err);
    }

    /* It is recommended to calibrate the SAADC at least once before use, and whenever the ambient temperature has changed by more than 10 °C */
    NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;

    /* Welcome message */
    printf("\n\r Illustration of the use of shmem + semaphores\n\r");

    /* Create and init semaphores */
    k_sem_init(&sem_ab, 0, 1);
    k_sem_init(&sem_bc, 0, 1);

    /* Create tasks */
    thread_sensor_tid = k_thread_create(&thread_sensor_data, thread_sensor_stack,
                                        K_THREAD_STACK_SIZEOF(thread_sensor_stack), thread_sensor_code,
                                        NULL, NULL, NULL, thread_sensor_prio, 0, K_NO_WAIT);

    thread_processing_tid = k_thread_create(&thread_processing_data, thread_processing_stack,
                                            K_THREAD_STACK_SIZEOF(thread_processing_stack), thread_processing_code,
                                            NULL, NULL, NULL, thread_processing_prio, 0, K_NO_WAIT);

    thread_processing_tid = k_thread_create(&thread_output_data, thread_output_stack,
                                            K_THREAD_STACK_SIZEOF(thread_output_stack), thread_output_code,
                                            NULL, NULL, NULL, thread_output_prio, 0, K_NO_WAIT);

    return;
}

/* Thread code implementation */
void thread_sensor_code(void *argA, void *argB, void *argC)
{
    /* Timing variables to control task periodicity */
    int64_t fin_time = 0, release_time = 0;

    /* Other variables */
    long int nact = 0;

    printk("Thread A init (periodic)\n");

    /* Compute next release instant */
    release_time = k_uptime_get() + SAMP_PERIOD_MS;
    int err = 0;

    /* Thread loop */
    while (1)
    {

        /* Do the workload */
        printk("\n\nThread A instance %ld released at time: %lld (ms). \n", ++nact, k_uptime_get());

        err = adc_sample();
        if (err)
        {
            printk("adc_sample() failed with error code %d\n\r", err);
        }
        else
        {
            if (adc_sample_buffer[0] > 1023)
            {
                printk("adc reading out of range\n\r");
            }
            else
            {
                /* ADC is set to use gain of 1/4 and reference VDD/4, so input range is 0...VDD (3 V), with 10 bit resolution */
                printk("adc reading: raw:%4u / %4u mV: \n\r", adc_sample_buffer[0], (uint16_t)(1000 * adc_sample_buffer[0] * ((float)3 / 1023)));
                ab = (uint16_t)(1000 * adc_sample_buffer[0] * ((float)3 / 1023));
                ab = 8000;
                printk("Thread A set ab value to: %d \n", ab);
            }
        }

        k_sem_give(&sem_ab);

        /* Wait for next release instant */
        fin_time = k_uptime_get();
        if (fin_time < release_time)
        {
            k_msleep(release_time - fin_time);
            release_time += SAMP_PERIOD_MS;
        }
    }
}

void thread_processing_code(void *argA, void *argB, void *argC)
{
    /* Other variables */
    long int nact = 0;
    uint16_t precedents[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t current_read = 0;
    int iterations = 0;

    printk("Thread B init (sporadic, waits on a semaphore by task A)\n");
    while (1)
    {
        k_sem_take(&sem_ab, K_FOREVER);

        printk("Thread B instance %ld released at time: %lld (ms). \n", ++nact, k_uptime_get());
        printk("Task B read ab value: %d\n", ab);
        current_read = ab;

        if (iterations > 8)
        {
            uint16_t avg = precedentsAverage(precedents, current_read, 9);
            bc = avg;
            k_sem_give(&sem_bc);
            printk("avg([%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]]) = %d\n", precedents[0], precedents[1], precedents[2], precedents[3], precedents[4], precedents[5], precedents[6], precedents[7], precedents[8], current_read, avg);

            precedents[iterations % 9] = current_read;
        }
        else
            precedents[iterations] = current_read;
        iterations++;
        printk("Thread B set bc value to: %d \n", bc);
    }
}

void thread_output_code(void *argA, void *argB, void *argC)
{
    /* Other variables */
    long int nact = 0;
    int ret;

    printk("Thread C init (sporadic, waits on a semaphore by task A)\n");

    /* Check if device is ready */
    if (!device_is_ready(led0.port) || !device_is_ready(led1.port) || !device_is_ready(led2.port) || !device_is_ready(led3.port))
    {
        return;
    }

    while (1)
    {
        k_sem_take(&sem_bc, K_FOREVER);
        printk("Thread C instance %5ld released at time: %lld (ms). \n", ++nact, k_uptime_get());
        printk("Task C read bc value: %d\n", bc);

        printk("\t%d\n",bc);
        if (bc < 1000)
        {
            ret = gpio_pin_set_dt(&led0, 1);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            ret = gpio_pin_set_dt(&led1, 1);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            ret = gpio_pin_set_dt(&led2, 1);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            ret = gpio_pin_set_dt(&led3, 1);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            
        }
        else if (bc < 2000)
        {
            ret = gpio_pin_set_dt(&led0, 1);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            ret = gpio_pin_set_dt(&led1, 1);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            ret = gpio_pin_set_dt(&led2, 1);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            ret = gpio_pin_set_dt(&led3, 0);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
        }
        else if (bc < 3000)
        {
            ret = gpio_pin_set_dt(&led0, 1);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            ret = gpio_pin_set_dt(&led1, 1);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            ret = gpio_pin_set_dt(&led2, 0);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            ret = gpio_pin_set_dt(&led3, 0);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
        }
        else
        {
            ret = gpio_pin_set_dt(&led0, 1);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            ret = gpio_pin_set_dt(&led1, 0);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            ret = gpio_pin_set_dt(&led2, 0);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
            ret = gpio_pin_set_dt(&led3, 0);
            if (ret < 0)
                printk("SETTING LED VALUE FAILED");
        }

       
        
    }
}

int precedentsAverage(uint16_t *precedents, uint16_t current_read, uint16_t precedents_size)
{
    int sum = 0;
    for (uint16_t i = 0; i < precedents_size; i++)
        sum += precedents[i];
    return (sum + current_read) / (precedents_size+1);
}

