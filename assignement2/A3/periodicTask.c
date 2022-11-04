/* ************************************************************
 * Xenomai - creates a periodic task
 *
 * Paulo Pedreiras
 * 	Out/2020: Upgraded from Xenomai V2.5 to V3.1
 *
 ************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>

#include <sys/mman.h> // For mlockall

// Xenomai API (former Native API)
#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <alchemy/queue.h>

#define MS_2_NS(ms) (ms * 1000 * 1000) /* Convert ms to ns */

/* *****************************************************
 * Define task structure for setting input arguments
 * *****************************************************/
struct taskArgsStruct
{
    RTIME taskPeriod_ns;
    char *taskName;
};

/* *******************
 * Task attributes
 * *******************/
#define TASK_MODE 0  // No flags
#define TASK_STKSZ 0 // Default stack size

#define TASK_A_PRIO 26 // RT priority [0..99]
#define ACK_PERIOD_MS MS_2_NS(1000)

#define TASK_B_PRIO 25 // RT priority [0..99]

#define TASK_C_PRIO 24 // RT priority [0..99]

int read_value = 0;

RT_TASK sensor_task, processing_task, storage_task; // Task decriptor
RT_QUEUE processing_q, storage_q;
/* *********************
 * Function prototypes
 * **********************/
void catch_signal(int sig); /* Catches CTRL + C to allow a controlled termination of the application */
void wait_for_ctrl_c(void);
void Heavy_Work(void);                 /* Load task */
void processing_task_code(void *args); /* Task body */
void sensor_task_code(void *args);     /* Task body */
void storage_task_code(void *args);    /* Task body */
u_int16_t precedentsAverage(u_int16_t* precedents, u_int16_t current_read);
/* ******************
 * Main function
 * *******************/
int main(int argc, char *argv[])
{
    // rt_queue_create(&processing_q,"queue",50,Q_UNLIMITED,Q_FIFO);
    // rt_queue_create(&storage_q,"queue",50,Q_UNLIMITED,Q_FIFO);

    // printf("%p",msg);

    int err;
    struct taskArgsStruct taskAArgs, taskBArgs, taskCArgs;
    /* Lock memory to prevent paging */
    mlockall(MCL_CURRENT | MCL_FUTURE);

    taskAArgs.taskPeriod_ns = ACK_PERIOD_MS;
    taskAArgs.taskName = "Sensor";
    taskBArgs.taskPeriod_ns = 0;
    taskBArgs.taskName = "Processing";
    taskCArgs.taskPeriod_ns = 0;
    taskCArgs.taskName = "Storage";

    /* Create RT Queues */
    err = rt_queue_create(&processing_q, "processing_q", 10000, Q_UNLIMITED, Q_FIFO);
    if (err)
    {
        printf("Error creating Processing queue! (error code = %d)\n", err);
        return err;
    }
    else
        printf("Processing queue created successfully\n");

    err = rt_queue_create(&storage_q, "storage_q", 10000, Q_UNLIMITED, Q_FIFO);
    if (err)
    {
        printf("Error creating Processing queue! (error code = %d)\n", err);
        return err;
    }
    else
        printf("Processing queue created successfully\n");

    /* Create RT tasks */
    /* Args: descriptor, name, stack size, priority [0..99] and mode (flags for CPU, FPU, joinable ...) */
    err = rt_task_create(&sensor_task, "Task Sensor", TASK_STKSZ, TASK_A_PRIO, TASK_MODE);
    if (err)
    {
        printf("Error creating %s task! (error code = %d)\n", taskAArgs.taskName, err);
        return err;
    }
    else
        printf("Task %s created successfully\n", taskAArgs.taskName);
    err = rt_task_create(&processing_task, "Task Processing", TASK_STKSZ, TASK_A_PRIO, TASK_MODE);
    if (err)
    {
        printf("Error creating %s task! (error code = %d)\n", taskBArgs.taskName, err);
        return err;
    }
    else
        printf("Task %s created successfully\n", taskBArgs.taskName);
    err = rt_task_create(&storage_task, "Task Storage", TASK_STKSZ, TASK_A_PRIO, TASK_MODE);
    if (err)
    {
        printf("Error creating %s task! (error code = %d)\n", taskCArgs.taskName, err);
        return err;
    }
    else
        printf("Task %s created successfully\n", taskCArgs.taskName);

    /* Start RT task */
    /* Args: task decriptor, address of function/implementation and argument*/

    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(0, &cpus);
    rt_task_set_affinity(&sensor_task, &cpus);
    rt_task_set_affinity(&processing_task, &cpus);
    rt_task_set_affinity(&storage_task, &cpus);
    rt_task_start(&sensor_task, &sensor_task_code, (void *)&taskAArgs);
    rt_task_start(&processing_task, &processing_task_code, (void *)&taskBArgs);
    rt_task_start(&storage_task, &storage_task_code, (void *)&taskCArgs);

    /* wait for termination signal */
    wait_for_ctrl_c();

    return 0;
}

void storage_task_code(void *args)
{
    FILE *sensor_data = fopen("newSensorData.txt", "w");
    ssize_t len;
    void *msg;
    int err;
    /* Bind to a queue which has been created elsewhere, either in
       kernel or user-space. The call will block us until such queue
       is created with the expected name. The queue should have been
       created with the Q_SHARED mode set, which is implicit when
       creation takes place in user-space. */
    err = rt_queue_bind(&storage_q, "storage_q", TM_INFINITE);
    while ((len = rt_queue_receive(&storage_q, &msg, TM_INFINITE)) > 0) {
        fwrite(msg, sizeof(u_int16_t), 1, sensor_data);
        rt_queue_free(&storage_q, msg);
    }
    rt_queue_unbind(&storage_q);
    fclose(sensor_data);
}

void processing_task_code(void *args)
{
    ssize_t len;
    void *msg;
    int err;
    /* Bind to a queue which has been created elsewhere, either in
       kernel or user-space. The call will block us until such queue
       is created with the expected name. The queue should have been
       created with the Q_SHARED mode set, which is implicit when
       creation takes place in user-space. */
    err = rt_queue_bind(&processing_q, "processing_q", TM_INFINITE);
    if (err)
        printf("fail");
    /* Collect each message sent to the queue by the queuer() routine,
       until the queue is eventually removed from the system by a call
       to rt_queue_delete(). */
    u_int16_t precedents[4] = {0, 0, 0, 0};
    int message_counter = 0;
    u_int16_t current_read = 0;
    while ((len = rt_queue_receive(&processing_q, &msg, TM_INFINITE)) > 0)
    {
        current_read = *((uint16_t *)msg);
        if (message_counter > 3)
        {
            // calculate the average with the new elem and the precedents
            u_int16_t avg = precedentsAverage(precedents, current_read);
            void *msg;
            printf("avg([%d,%d,%d,%d,%d]) = %d\n", precedents[0], precedents[1], precedents[2], precedents[3], current_read, avg);
            /* Get a message block of the right size. */
            msg = rt_queue_alloc(&storage_q, sizeof(uint16_t));

            if (!msg)
                /* No memory available. */
                printf("No memory available!\n");

            memcpy(msg, &current_read, sizeof(uint16_t));
            int a = rt_queue_send(&storage_q, msg, sizeof(uint16_t), Q_NORMAL);
            precedents[message_counter % 4] = current_read;
        }
        else
            precedents[message_counter] = current_read;
        // printf("received message> len=%li bytes, ptr=%p, s=%d\n", len, msg, *((uint16_t *)msg));
        rt_queue_free(&processing_q, msg);
        message_counter++;
    }
    /* We need to unbind explicitly from the queue in order to
       properly release the underlying memory mapping. Exiting the
       process unbinds all mappings automatically. */
    rt_queue_unbind(&processing_q);

    if (len != -EIDRM)
        /* We received some unexpected error notification. */
        printf("fail");
}

/* ***********************************
 * Task body implementation
 * *************************************/
void sensor_task_code(void *args)
{
    // Sensor file
    FILE *sensor_file = fopen("sensorData.txt", "r");

    RT_TASK *curtask;
    RT_TASK_INFO curtaskinfo;
    struct taskArgsStruct *taskArgs;

    RTIME ta = 0, ta_prev = 0, t_min = ACK_PERIOD_MS, t_max = ACK_PERIOD_MS;
    unsigned long overruns;
    int err;
    /* Get task information */
    curtask = rt_task_self();
    rt_task_inquire(curtask, &curtaskinfo);
    taskArgs = (struct taskArgsStruct *)args;
    char *proc_task_name = calloc(100, sizeof(char));
    strcpy(proc_task_name, curtaskinfo.name);
    strcat(proc_task_name, " - ");
    strcat(proc_task_name, taskArgs->taskName);
    printf("Task %s init, period:%llu\n", proc_task_name, taskArgs->taskPeriod_ns);
    /* Set task as periodic */
    err = rt_task_set_periodic(NULL, TM_NOW, taskArgs->taskPeriod_ns);
    for (;;)
    {
        err = rt_task_wait_period(&overruns);
        ta = rt_timer_read();
        if (err)
        {
            printf("task %s overrun!!!\n", proc_task_name);
            break;
        }
        if (ta_prev == 0)
        {
            printf("Task %s activation at time %llu\n", proc_task_name, ta);
        }
        else
        {
            if (ta - ta_prev < t_min)
                t_min = ta - ta_prev;
            else if (ta - ta_prev > t_max)
                t_max = ta - ta_prev;
            // printf("Task %s inter-arrival time (us): min: %10.3f / max: %10.3f \n\r", proc_task_name, (float)t_min / 1000, (float)t_max / 1000);
        }
        ta_prev = ta;

        u_int16_t sensor_data;
        if (sensor_file != NULL && !feof(sensor_file))
            fread(&sensor_data, sizeof(sensor_data), 1, sensor_file);
        else
            break;

        void *msg;

        /* Get a message block of the right size. */
        msg = rt_queue_alloc(&processing_q, sizeof(uint16_t));

        if (!msg)
            /* No memory available. */
            printf("No memory available!\n");

        memcpy(msg, &sensor_data, sizeof(uint16_t));
        int a = rt_queue_send(&processing_q, msg, sizeof(uint16_t), Q_NORMAL);

        /* Task "load" */
        Heavy_Work();
    }

    printf("Sensor task finished!\n");
    fclose(sensor_file);
    return;
}

/* **************************************************************************
 *  Catch control+c to allow a controlled termination
 * **************************************************************************/
void catch_signal(int sig)
{
    return;
}

void wait_for_ctrl_c(void)
{
    signal(SIGTERM, catch_signal); // catch_signal is called if SIGTERM received
    signal(SIGINT, catch_signal);  // catch_signal is called if SIGINT received

    // Wait for CTRL+C or sigterm
    pause();

    // Will terminate
    printf("Terminating ...\n");
}

/* **************************************************************************
 *  Task load implementation. In the case integrates numerically a function
 * **************************************************************************/
#define f(x) 1 / (1 + pow(x, 2)) /* Define function to integrate*/
void Heavy_Work(void)
{
    float lower, upper, integration = 0.0, stepSize, k;
    int i, subInterval;

    RTIME ts, // Function start time
        tf;   // Function finish time
    
    static int first = 0; // Flag to signal first execution

    /* Get start time */
    ts = rt_timer_read();

    /* Integration parameters */
    /*These values can be tunned to cause a desired load*/
    lower = 0;
    upper = 100;
    subInterval = 1000000;

    /* Calculation */
    /* Finding step size */
    stepSize = (upper - lower) / subInterval;

    /* Finding Integration Value */
    integration = f(lower) + f(upper);
    for (i = 1; i <= subInterval - 1; i++)
    {
        k = lower + i * stepSize;
        integration = integration + 2 * f(k);
    }
    integration = integration * stepSize / 2;

    /* Get finish time and show results */
    if (!first)
    {
        tf = rt_timer_read();
        tf -= ts; // Compute time difference form start to finish

        printf("Integration value is: %.3f. It took %9llu ns to compute.\n", integration, tf);

        first = 1;
    }
}

u_int16_t precedentsAverage(u_int16_t* precedents, u_int16_t current_read) {
    return (precedents[0] + precedents[1] + precedents[2] + precedents[3] + current_read) / 5;  
}