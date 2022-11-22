
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
#define thread_button_prio 2

/* Therad periodicity (in ms)*/
#define SAMP_PERIOD_MS 1000
#define SLEEP_TIME_MS 500

#define LED0_NODE DT_NODELABEL(led0)
#define LED1_NODE DT_NODELABEL(led1)
#define LED2_NODE DT_NODELABEL(led2)
#define LED3_NODE DT_NODELABEL(led3)
#define SW0_NODE DT_NODELABEL(button0)

/* Create thread stack space */
K_THREAD_STACK_DEFINE(thread_sensor_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_processing_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_output_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_button_stack, STACK_SIZE);

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

/* Create variables for thread data */
struct k_thread thread_sensor_data;
struct k_thread thread_processing_data;
struct k_thread thread_output_data;
struct k_thread thread_button_data;

/* Create task IDs */
k_tid_t thread_sensor_tid;
k_tid_t thread_processing_tid;
k_tid_t thread_output_tid;
k_tid_t thread_button_tid;

/* Global vars (shared memory between tasks A/B and B/C, resp) */
uint16_t sensor_processing = 100;
uint16_t processing_output = 200;

/* Semaphores for task synch */
struct k_sem sem_sensor_processing;
struct k_sem sem_processing_output;
struct k_sem sem_button;

/* Thread code prototypes */
void thread_sensor_code(void *argA, void *argB, void *argC);
void thread_processing_code(void *argA, void *argB, void *argC);
void thread_output_code(void *argA, void *argB, void *argC);
void thread_button_code(void *argA, void *argB, void *argC);
int precedentsAverage(uint16_t *precedents, uint16_t current_read, uint16_t precedents_size);
/* Define a variable of type static struct gpio_callback, which will latter be used to install the callback
 *  It defines e.g. which pin triggers the callback
 */
static struct gpio_callback button_cb_data;

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

/* Define a callback function. It is like an ISR that is called when the button is pressed */
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{

    k_sem_give(&sem_button);
}

/* Main function */
void main(void)
{
    int err = 0;
    int ret;

    /* Check if device is ready */
    if (!device_is_ready(led0.port) || !device_is_ready(led1.port) || !device_is_ready(led2.port) || !device_is_ready(led3.port))
    {
        printk("Error: leds are not ready\n");
        return;
    }

    if (!device_is_ready(button.port))
    {
        printk("Error: button device %s is not ready\n", button.port->name);
        return;
    }

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

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0)
    {
        printk("Error: gpio_pin_configure_dt failed for button, error:%d", ret);
        return;
    }

    /* Configure the interrupt on the button's pin */
    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0)
    {
        printk("Error: gpio_pin_interrupt_configure_dt failed for button, error:%d", ret);
        return;
    }

    /* Initialize the static struct gpio_callback variable   */
    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));

    /* Add the callback function by calling gpio_add_callback()   */
    gpio_add_callback(button.port, &button_cb_data);

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
    k_sem_init(&sem_sensor_processing, 0, 1);
    k_sem_init(&sem_processing_output, 0, 1);
    k_sem_init(&sem_button, 0, 1);

    /* Create tasks */
    thread_sensor_tid = k_thread_create(&thread_sensor_data, thread_sensor_stack,
                                        K_THREAD_STACK_SIZEOF(thread_sensor_stack), thread_sensor_code,
                                        NULL, NULL, NULL, thread_sensor_prio, 0, K_NO_WAIT);

    thread_processing_tid = k_thread_create(&thread_processing_data, thread_processing_stack,
                                            K_THREAD_STACK_SIZEOF(thread_processing_stack), thread_processing_code,
                                            NULL, NULL, NULL, thread_processing_prio, 0, K_NO_WAIT);

    thread_output_tid = k_thread_create(&thread_output_data, thread_output_stack,
                                        K_THREAD_STACK_SIZEOF(thread_output_stack), thread_output_code,
                                        NULL, NULL, NULL, thread_output_prio, 0, K_NO_WAIT);

    thread_button_tid = k_thread_create(&thread_button_data, thread_button_stack,
                                        K_THREAD_STACK_SIZEOF(thread_button_stack), thread_button_code,
                                        NULL, NULL, NULL, thread_button_prio, 0, K_NO_WAIT);
    return;
}

/* Thread code implementation */
void thread_sensor_code(void *argA, void *argB, void *argC)
{
    /* Timing variables to control task periodicity */
    int64_t fin_time = 0, release_time = 0;

    /* Other variables */
    long int nact = 0;

    printk("Thread sensor init (periodic)\n");

    /* Compute next release instant */
    release_time = k_uptime_get() + SAMP_PERIOD_MS;
    int err = 0;

    /* Thread loop */
    while (1)
    {

        /* Do the workload */
        printk("\n\nThread sensor instance %ld released at time: %lld (ms). \n", ++nact, k_uptime_get());

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
                sensor_processing = (uint16_t)(1000 * adc_sample_buffer[0] * ((float)3 / 1023));
                printk("Thread sensor set sensor_processing value to: %d \n", sensor_processing);
            }
        }

        k_sem_give(&sem_sensor_processing);

        /* Wait for next release instant */
        fin_time = k_uptime_get();

        // when the task is interrupted by pressing button 0 (test button)
        if(fin_time - release_time > SAMP_PERIOD_MS){
            release_time += SAMP_PERIOD_MS*5;
        }
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

    printk("Thread processing  init (sporadic, waits on a semaphore by task A)\n");
    while (1)
    {
        k_sem_take(&sem_sensor_processing, K_FOREVER);

        printk("Thread processing  instance %ld released at time: %lld (ms). \n", ++nact, k_uptime_get());
        printk("Task processing read sensor_processing value: %d\n", sensor_processing);
        current_read = sensor_processing;

        if (iterations > 8)
        {
            uint16_t avg = precedentsAverage(precedents, current_read, 9);
            processing_output = avg;
            k_sem_give(&sem_processing_output);
            printk("avg([%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]]) = %d\n", precedents[0], precedents[1], precedents[2], precedents[3], precedents[4], precedents[5], precedents[6], precedents[7], precedents[8], current_read, avg);

            precedents[iterations % 9] = current_read;
        }
        else
            precedents[iterations] = current_read;
        iterations++;
        printk("Thread processing  set processing_output value to: %d \n", processing_output);
    }
}

void thread_output_code(void *argA, void *argB, void *argC)
{
    /* Other variables */
    long int nact = 0;
    int ret;

    printk("Thread output init (sporadic, waits on a semaphore by task A)\n");

    while (1)
    {
        k_sem_take(&sem_processing_output, K_FOREVER);
        printk("Thread output instance %5ld released at time: %lld (ms). \n", ++nact, k_uptime_get());
        printk("Task output read processing_output value: %d\n", processing_output);

        if (processing_output < 1000)
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
        else if (processing_output < 2000)
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
        else if (processing_output < 3000)
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

void thread_button_code(void *argA, void *argB, void *argC)
{
    /* Other variables */
    int ret;

    printk("Thread button init (sporadic, waits on a semaphore by button pressed callback)\n");

    while (1)
    {
        k_sem_take(&sem_button, K_FOREVER);

        k_thread_suspend(thread_sensor_tid);
        k_thread_suspend(thread_processing_tid);
        k_thread_suspend(thread_output_tid);

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

        for (size_t i = 0; i < 10; i++)
        {
            gpio_pin_toggle_dt(&led0);
            gpio_pin_toggle_dt(&led1);
            gpio_pin_toggle_dt(&led2);
            gpio_pin_toggle_dt(&led3);
            k_msleep(SLEEP_TIME_MS);
        }

        k_thread_resume(thread_sensor_tid);
        k_thread_resume(thread_processing_tid);
        k_thread_resume(thread_output_tid);
    }
}

int precedentsAverage(uint16_t *precedents, uint16_t current_read, uint16_t precedents_size)
{
    int sum = 0;
    for (uint16_t i = 0; i < precedents_size; i++)
        sum += precedents[i];
    return (sum + current_read) / (precedents_size + 1);
}
