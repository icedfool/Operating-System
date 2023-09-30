#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "queue.h"

typedef enum
{
    preempt,
    c_switch_out,
    cpu_end,
    cpu_start,
    waiting,
    arrived,
    c_switch_in,
    terminated,
} mode;

typedef enum
{
    false,
    true
} boolean;

typedef struct
{
    int next;
    int cpu_index;
    int io_index;
    char pid;
    int arrival;
    int total_burst;
    int tau;
    int *cpu_time;
    int *io_time;
    int remaining;
    int remaining_tau;
    int time_expire;
    mode next_status;

    // variables used to track:
    double total_cpu_wait_t;
    double queue_in;
} process;

int p_num;
process *p_array;
char alphabet[26] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

void fcfs();
void sjf();
void srt();
void rr();
void next_exp();
process *initial();
void display();
int recalculate();
int comparator();
void queue_sort_tau();
void sync_array();
process *compare();
void reseed();

// Functions for printing summary:
double get_avg_burst_time();
double get_avg_wait_time();
double get_tr_time();
FILE *printing_sum();

// memory cleaning:
void p_array_free();

int main(int argc, char **argv)
{
    //   printf("%d\n,argc);
    // error handling
    if (argc != 8)
    {
        perror("Invalid argument count\n");
        return EXIT_FAILURE;
    }

    // initialize file out steam:
    FILE *output = printing_sum();

    // arguments
    p_num = atoi(argv[1]);
    int seed = atoi(argv[2]);
    double lambda = atof(argv[3]);
    int upper = atoi(argv[4]);
    int time_cs = atoi(argv[5]);
    double alpha = atof(argv[6]);
    int time_slice = atoi(argv[7]);
    // p_array = calloc(p_num, sizeof(process));
    p_array = initial(seed, lambda, upper);

    // special message
    display();
    printf("\n");
    fcfs(time_cs, output);
    reseed(lambda);
    printf("\n");
    sjf(time_cs, alpha, output);
    reseed(lambda);
    printf("\n");
    //srt(time_cs, alpha);
    reseed(lambda);
    printf("\n");
    rr(time_cs,time_slice,output);

    p_array_free();

    return EXIT_SUCCESS;
}

// Initialize the file outsteam:
FILE *printing_sum()
{
    const char *filename = "simout.txt"; // NULL means "use stdout"
    FILE *outStream;

    if (filename)
    {
        FILE *out = fopen(filename, "w");
        if (out)
        {
            outStream = out;
        }
        else
        {
            outStream = stdout;
        }
    }
    else
    {
        outStream = stdout;
    }

    return outStream;
}

// find average bursts time:
double get_avg_burst_time()
{
    // initialize the two accumulated vals:
    float accu_burst_t, accu_burst;
    accu_burst_t = accu_burst = 0;

    for (int i = 0; i < p_num; i++)
    {
        // adding the all burst times:
        for (int j = 0; j < p_array[i].total_burst; j++)
        {
            accu_burst_t += p_array[i].cpu_time[j];
        }
        accu_burst += p_array[i].total_burst;
    }
    double output = ceil(accu_burst_t / accu_burst * 1000.0) / 1000.0;
    return output;
}

double get_avg_wait_time()
{
    double accu_wait_t, accu_burst;
    accu_wait_t = accu_burst = 0;

    // adding the all the wait times:
    for (int i = 0; i < p_num; i++)
    {
        accu_wait_t += p_array[i].total_cpu_wait_t;
        accu_burst += p_array[i].total_burst;
    }
    double output = ceil(accu_wait_t / accu_burst * 1000.0) / 1000.0;
    return output;
}

// find the average tr time:
double get_tr_time(int input)
{
    double tt_burst = 0;
    for (int i = 0; i < p_num; i++)
    {
        tt_burst += p_array[i].total_burst;
    }
    double output = ceil(input / tt_burst * 1000.0) / 1000.0;
    return output;
}

void p_array_free()
{
    // free all the array in the processes:
    for (int i = 0; i < p_num; i++)
    {
        free(p_array[i].cpu_time);
        free(p_array[i].io_time);
    }
    free(p_array);
}

// find processes' data
process *initial(int seed, double lambda, int upper)
{

    process *queue = calloc(p_num, sizeof(process));
    srand48(seed);

    // loop all process
    for (int i = 0; i < p_num; i++)
    {
        process current;
        current.pid = alphabet[i];
        current.cpu_index = current.io_index = current.remaining = current.remaining_tau = 0;
        current.next_status = arrived;
        current.tau = 1 / lambda;
        current.time_expire = -1;

        // find initial arrival time
        double r = drand48();
        int arrival = floor(-log(r) / lambda);
        while (arrival > upper)
        {
            arrival = -log(drand48()) / lambda;
        }
        current.arrival = current.next = arrival;

        // find number total burst
        int total_burst = ceil(drand48() * 100);
        current.total_burst = total_burst;

        // find all cpu burst time and io burst time
        current.cpu_time = calloc(total_burst, sizeof(int));
        current.io_time = calloc(total_burst - 1, sizeof(int));
        for (int j = 0; j < total_burst; j++)
        {
            int cpu_time = ceil(-log(drand48()) / lambda);
            while (cpu_time > upper)
            {
                cpu_time = ceil(-log(drand48()) / lambda);
            }
            current.cpu_time[j] = cpu_time;
            if (j != total_burst - 1)
            {
                int io_time = ceil(-log(drand48()) / lambda);
                while (io_time > upper)
                {
                    io_time = ceil(-log(drand48()) / lambda);
                }
                current.io_time[j] = io_time * 10;
            }
        }

        // helper variables to keep intrack:
        current.total_cpu_wait_t = 0;

        queue[i] = current;
    }
    return queue;
}

// print process information
void display()
{
    for (int i = 0; i < p_num; i++)
    {
        process current = p_array[i];
        printf("Process %c: arrival time %dms; tau %dms; %d CPU bursts:\n", current.pid, current.arrival, current.tau, current.total_burst);
        for (int j = 0; j < current.total_burst; j++)
        {
            printf("--> CPU burst %dms", current.cpu_time[j]);
            if (j != current.total_burst - 1)
            {
                printf(" --> I/O burst %dms", current.io_time[j]);
            }
            printf("\n");
        }
    }
}

// reset process to initial state
void reseed(double lambda)
{

    for (int i = 0; i < p_num; i++)
    {
        p_array[i].next = p_array[i].arrival;
        p_array[i].cpu_index = p_array[i].io_index = 0;
        p_array[i].tau = 1 / lambda;
        p_array[i].remaining = p_array[i].remaining_tau = 0;
        p_array[i].time_expire = -1;
        p_array[i].next_status = arrived;
        p_array[i].total_cpu_wait_t = 0;
    }
}

// recalculate tau
int recalculate(double alpha, int tau, int actual)
{
    return ceil((alpha * actual) + ((1 - alpha) * tau));
}

// FCFS
void fcfs(int context_switch, FILE *fp)
{

    // data types
    int time = 0;
    boolean CPU_IN_USE = false;
    int cpu_end_time = 0;
    queue q;
    queue_init(&q, p_num);
    int term = 0;
    int accu_turn_around_time = 0;
    printf("time 0ms: Simulator started for FCFS ");
    printing(&q);

    // calculate the total cpu time:
    int total_cpu_use = 0;
    for (int i = 0; i < p_num; i++)
    {
        for (int j = 0; j < p_array[i].total_burst; j++)
        {
            total_cpu_use += p_array[i].cpu_time[j];
        }
    }

    // finding total number of context switching:
    int numb_cs = 0;
    for (int i = 0; i < p_num; i++)
    {
        numb_cs += p_array[i].total_burst;
    }

    // Terminate simulation when all process terminates
    while (term != p_num)
    {

        // loop for handling ties
        for (int m = c_switch_out; m < terminated; m++)
        {
            // loop process
            for (int i = 0; i < p_num; i++)
            {
                // check if process has special event at this time
                if (p_array[i].next == time && p_array[i].next_status == m)
                {

                    switch (m)
                    {
                    // process arrived to ready queue
                    // update process next special event time
                    // update process next status
                    // update queue
                    case arrived:

                        if (q.current_size != 0)
                        {
                            char c = q.queue[(q.current_size) - 1];
                            for (int j = 0; j < p_num; j++)
                            {
                                if (p_array[j].pid == c)
                                {
                                    p_array[i].next = p_array[j].next + p_array[j].cpu_time[p_array[j].cpu_index] + context_switch;

                                    // update wait time:
                                    p_array[i].total_cpu_wait_t += p_array[i].next - time;
                                    accu_turn_around_time += (p_array[i].next - time) + p_array[i].cpu_time[p_array[i].io_index] + context_switch;
                                }
                            }
                        }
                        else
                        {
                            if (CPU_IN_USE)
                            {
                                p_array[i].next = cpu_end_time + context_switch / 2;

                                p_array[i].total_cpu_wait_t += p_array[i].next - time;
                                accu_turn_around_time += (p_array[i].next - time) + p_array[i].cpu_time[p_array[i].io_index] + context_switch;
                            }
                            else
                            {
                                p_array[i].next = time;
                                accu_turn_around_time += p_array[i].cpu_time[p_array[i].io_index] + context_switch;
                            }
                        }
                        p_array[i].next_status = c_switch_in;
                        adding_one(&q, p_array[i].pid);
                        // print message
                        printf("time %dms: Process %c arrived; added to ready queue ", time, p_array[i].pid);
                        printing(&q);
                        break;

                    // context switch into CPU
                    // assert current process is the first in queue
                    // update CPU status and CPU ending time
                    // update process next special event time
                    case c_switch_in:

                        if (!CPU_IN_USE && q.queue[0] == p_array[i].pid)
                        {
                            deleting_first(&q);
                            p_array[i].next = time + context_switch / 2;
                            p_array[i].next_status = cpu_start;
                            cpu_end_time = p_array[i].next + p_array[i].cpu_time[p_array[i].cpu_index];
                            CPU_IN_USE = true;
                        }
                        break;

                        // CPU start processing
                        // print message
                        // update process next special event time
                        // update cpu end time
                    case cpu_start:

                        printf("time %dms: Process %c started using the CPU for %dms burst ", time, p_array[i].pid, p_array[i].cpu_time[p_array[i].cpu_index]);
                        printing(&q);
                        p_array[i].next += p_array[i].cpu_time[p_array[i].cpu_index];
                        p_array[i].next_status = cpu_end;
                        break;

                        // CPU finish processing
                        // check if process terminates or continue to I/O
                    case cpu_end:
                        p_array[i].cpu_index++;
                        if (p_array[i].total_burst == p_array[i].cpu_index)
                        {
                            printf("time %dms: Process %c terminated ", time, p_array[i].pid);
                            printing(&q);
                            term++;
                        }
                        else
                        {
                            p_array[i].next = time + context_switch / 2;
                            printf("time %dms: Process %c completed a CPU burst; %d bursts to go ", time, p_array[i].pid, p_array[i].total_burst - p_array[i].cpu_index);
                            printing(&q);
                            printf("time %dms: Process %c switching out of CPU; will block on I/O until time %dms ", time, p_array[i].pid, p_array[i].next + p_array[i].io_time[p_array[i].io_index]);
                            printing(&q);
                        }
                        p_array[i].next_status = c_switch_out;
                        break;

                    // context switch out of CPU
                    // update next special event time
                    case c_switch_out:
                        if (p_array[i].total_burst == p_array[i].cpu_index)
                        {
                            p_array[i].next_status = terminated;
                        }
                        else
                        {
                            p_array[i].next = time + p_array[i].io_time[p_array[i].io_index];
                            p_array[i].next_status = waiting;
                            p_array[i].io_index++;
                        }
                        CPU_IN_USE = false;
                        break;

                        // process leave I/O and add back to queue
                        // update next special event time
                        // print message
                    case waiting:
                        if (q.current_size != 0)
                        {
                            char c = q.queue[(q.current_size) - 1];
                            for (int j = 0; j < p_num; j++)
                            {
                                if (p_array[j].pid == c)
                                {
                                    p_array[i].next = p_array[j].next + p_array[j].cpu_time[p_array[j].cpu_index] + context_switch;

                                    // update sum:
                                    p_array[i].total_cpu_wait_t += p_array[i].next - time;
                                    accu_turn_around_time += (p_array[i].next - time) + p_array[i].cpu_time[p_array[i].io_index] + context_switch;
                                }
                            }
                        }
                        else
                        {
                            if (CPU_IN_USE)
                            {
                                p_array[i].next = cpu_end_time + context_switch / 2;
                                p_array[i].total_cpu_wait_t += p_array[i].next - time;
                                accu_turn_around_time += (p_array[i].next - time) + p_array[i].cpu_time[p_array[i].io_index] + context_switch;
                            }
                            else
                            {
                                p_array[i].next = time;
                                accu_turn_around_time += p_array[i].cpu_time[p_array[i].io_index] + context_switch;
                            }
                        }

                        adding_one(&q, p_array[i].pid);
                        p_array[i].next_status = c_switch_in;

                        // print message
                        printf("time %dms: Process %c completed I/O; added to ready queue ", time, p_array[i].pid);
                        printing(&q);

                        break;

                    case terminated:
                        break;
                    }
                }
            }
        }

        // update next earliest special event time
        int smallest_time = time;
        for (int i = 0; i < p_num; i++)
        {
            if (p_array[i].next_status != terminated && p_array[i].next > smallest_time)
            {
                smallest_time = p_array[i].next;
            }
        }
        for (int i = 0; i < p_num; i++)
        {
            if (p_array[i].next_status != terminated && p_array[i].next < smallest_time)
            {
                smallest_time = p_array[i].next;
                // printf("%c: %d\n", p_array[i].pid,p_array[i].next);
            }
        }

        time = smallest_time;
        // printf("%d\n",time);
    }
    printf("time %dms: Simulator ended for FCFS ", time + context_switch / 2);
    printing(&q);

    // Printing summary:
    double fcfs_avg_burst = get_avg_burst_time();
    double fcfs_avg_wait = get_avg_wait_time();
    double fcfs_avg_tr = get_tr_time(accu_turn_around_time);

    double f_burst = total_cpu_use, f_time = time + context_switch / 2;
    double cpu_ult = f_burst / f_time * 100;
    cpu_ult = ceil(cpu_ult * 1000.0) / 1000.0;

    fprintf(fp, "Algorithm FCES\n");
    fprintf(fp, "-- average CPU burst time: %.3f\n", fcfs_avg_burst);
    fprintf(fp, "-- average wait time: %.3f\n", fcfs_avg_wait);
    fprintf(fp, "-- average turnaround time: %.3f\n", fcfs_avg_tr);
    fprintf(fp, "-- total number of context switches: %d\n", numb_cs);
    fprintf(fp, "-- total number of preemptions: 0\n");
    fprintf(fp, "-- CPU utilization: %.3f%c\n", cpu_ult, 37);
    fprintf(fp, "\n");

    // free queue: 
    free (q.queue);
}

void sjf(int context_switch, double alpha, FILE *fp)
{
    int time = 0;
    boolean CPU_IN_USE = false;
    int cpu_end_time = 0;
    queue q;
    queue_init(&q, p_num);
    int term = 0;
    printf("time 0ms: Simulator started for SJF ");
    printing(&q);

    // find number of cpu bursts:
    int numb_cs = 0;
    for (int i = 0; i < p_num; i++)
    {
        numb_cs += p_array[i].total_burst;
    }

    // initialzie the total cpu use time first:
    int total_cpu_use = 0;

    // accumulated tr time:
    double accu_tr_time = 0;

    // Terminate simulation when all process terminates
    while (term != p_num)
    {

        for (int m = c_switch_out; m < terminated; m++)
        {
            // loop through all process
            for (int i = 0; i < p_num; i++)
            {
                // check if process has special event at this time
                if (p_array[i].next == time && p_array[i].next_status == m)
                {

                    switch (m)
                    {
                    // process arrived to ready queue
                    // update process next special event time
                    // update process next status
                    // update queue
                    case arrived:
                        if (q.current_size != 0)
                        {
                            char c = q.queue[(q.current_size) - 1];
                            for (int j = 0; j < p_num; j++)
                            {
                                if (p_array[j].pid == c)
                                {
                                    p_array[i].next = p_array[j].next + p_array[j].cpu_time[p_array[j].cpu_index] + context_switch;
                                    p_array[i].queue_in = time;
                                }
                            }
                        }
                        else
                        {
                            if (CPU_IN_USE)
                            {
                                p_array[i].next = cpu_end_time + context_switch / 2;
                                p_array[i].queue_in = time;
                            }
                            else
                            {
                                p_array[i].next = time;
                            }
                        }
                        p_array[i].next_status = c_switch_in;
                        adding_one(&q, p_array[i].pid);
                        if (q.current_size > 1)
                        {
                            queue_sort_tau(&q);

                            if (CPU_IN_USE)
                            {
                                sync_array(&q, cpu_end_time, context_switch, true);
                                // printf("time: %d\n",cpu_end_time);
                            }
                            else
                            {
                                sync_array(&q, time, context_switch, false);
                                // printf("time: %d\n",cpu_end_time);
                            }
                        }
                        p_array[i].queue_in = time;
                        printf("time %dms: Process %c (tau %dms) arrived; added to ready queue ", time, p_array[i].pid, p_array[i].tau);
                        printing(&q);
                        break;

                    case c_switch_in:

                        if (!CPU_IN_USE && q.queue[0] == p_array[i].pid)
                        {
                            deleting_first(&q);
                            // printf("%c, %d\n",p_array[i].pid, p_array[i].next);
                            p_array[i].next = time + context_switch / 2;
                            p_array[i].next_status = cpu_start;
                            cpu_end_time = p_array[i].next + p_array[i].cpu_time[p_array[i].cpu_index];
                            CPU_IN_USE = true;

                            total_cpu_use += cpu_end_time - time - (context_switch / 2);

                            // find the diff time of the process in the queue:
                            double diff_in_queue = time - p_array[i].queue_in;
                            p_array[i].total_cpu_wait_t += diff_in_queue;
                        }
                        break;
                    case cpu_start:

                        printf("time %dms: Process %c (tau %dms) started using the CPU for %dms burst ", time, p_array[i].pid, p_array[i].tau, p_array[i].cpu_time[p_array[i].cpu_index]);
                        printing(&q);
                        p_array[i].next += p_array[i].cpu_time[p_array[i].cpu_index];
                        p_array[i].next_status = cpu_end;
                        break;

                    case cpu_end:
                        p_array[i].cpu_index++;
                        p_array[i].next += context_switch / 2;
                        if (p_array[i].total_burst == p_array[i].cpu_index)
                        {
                            printf("time %dms: Process %c terminated ", time, p_array[i].pid);
                            printing(&q);
                            term++;

                            accu_tr_time += time - p_array[i].queue_in + context_switch / 2;
                        }
                        else
                        {
                            printf("time %dms: Process %c (tau %dms) completed a CPU burst; %d bursts to go ", time, p_array[i].pid, p_array[i].tau, p_array[i].total_burst - p_array[i].cpu_index);
                            printing(&q);

                            int old_tau = p_array[i].tau;
                            p_array[i].tau = recalculate(alpha, p_array[i].tau, p_array[i].cpu_time[p_array[i].cpu_index - 1]);

                            printf("time %dms: Recalculated tau for process %c: old tau %dms; new tau %dms ", time, p_array[i].pid, old_tau, p_array[i].tau);
                            printing(&q);

                            printf("time %dms: Process %c switching out of CPU; will block on I/O until time %dms ", time, p_array[i].pid, p_array[i].next + p_array[i].io_time[p_array[i].io_index]);
                            printing(&q);

                            accu_tr_time += time - p_array[i].queue_in + context_switch / 2;
                        }
                        p_array[i].next_status = c_switch_out;
                        // printf("running:%c %d %d\n",p_array[i].pid,p_array[i].next, p_array[i].next_status);
                        break;

                    case c_switch_out:

                        if (p_array[i].total_burst == p_array[i].cpu_index)
                        {
                            p_array[i].next_status = terminated;
                        }
                        else
                        {
                            p_array[i].next = time + p_array[i].io_time[p_array[i].io_index];
                            p_array[i].next_status = waiting;
                            p_array[i].io_index++;
                        }
                        CPU_IN_USE = false;

                        break;
                    case waiting:

                        if (q.current_size != 0)
                        {
                            char c = q.queue[(q.current_size) - 1];
                            for (int j = 0; j < p_num; j++)
                            {
                                if (p_array[j].pid == c)
                                {
                                    p_array[i].next = p_array[j].next + p_array[j].cpu_time[p_array[j].cpu_index] + context_switch;
                                }
                            }
                        }
                        else
                        {
                            if (CPU_IN_USE)
                            {
                                p_array[i].next = cpu_end_time + context_switch / 2;
                            }
                            else
                            {
                                p_array[i].next = time;
                            }
                        }

                        adding_one(&q, p_array[i].pid);
                        if (q.current_size > 1)
                        {
                            queue_sort_tau(&q);

                            if (CPU_IN_USE)
                            {
                                sync_array(&q, cpu_end_time, context_switch, true);
                                // printf("time: %d\n",cpu_end_time);
                            }
                            else
                            {
                                sync_array(&q, time, context_switch, false);
                                // printf("time: %d\n",cpu_end_time);
                            }
                        }
                        p_array[i].next_status = c_switch_in;

                        p_array[i].queue_in = time;

                        // print message
                        printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue ", time, p_array[i].pid, p_array[i].tau);
                        printing(&q);

                        break;

                    case terminated:
                        break;
                    }
                }
            }
        }

        // smallest time
        int smallest_time = time;
        for (int i = 0; i < p_num; i++)
        {
            if (p_array[i].next_status != terminated && p_array[i].next > smallest_time)
            {
                smallest_time = p_array[i].next;
            }
        }

        for (int i = 0; i < p_num; i++)
        {
            if (p_array[i].next_status != terminated && p_array[i].next < smallest_time)
            {
                smallest_time = p_array[i].next;
                // printf("%c: %d\n",p_array[i].pid, p_array[i].next);
            }
        }

        time = smallest_time;
        // printf("%d\n",time);
    }
    printf("time %dms: Simulator ended for SJF ", time);
    printing(&q);

    // printing summary message:
    double fcfs_avg_burst = get_avg_burst_time();
    double sjf_avg_wait = get_avg_wait_time();
    double sjf_tr_time = get_tr_time(accu_tr_time);

    // cpu utilization:
    double float_time = time, total_f = total_cpu_use;
    double cpu_u = (total_f / float_time) * 100;
    cpu_u = ceil(cpu_u * 1000.0) / 1000.0;

    fprintf(fp, "Algorithm SJF\n");
    fprintf(fp, "-- average CPU burst time: %.3f\n", fcfs_avg_burst);
    fprintf(fp, "-- average wait time: %.3f\n", sjf_avg_wait);
    fprintf(fp, "-- average turnaround time: %.3f\n", sjf_tr_time);
    fprintf(fp, "-- total number of context switches: %d\n", numb_cs);
    fprintf(fp, "-- total number of preemptions: 0\n");
    fprintf(fp, "-- CPU utilization: %.3f%c\n", cpu_u, 37);
    fprintf(fp, "\n");

    // free queue: 
    free (q.queue);
}

void srt(int context_switch, double alpha)
{
    int time = 0;
    boolean CPU_IN_USE = false;
    int cpu_end_time = 0;
    queue q;
    queue_init(&q, p_num);
    int term = 0;
    printf("time 0ms: Simulator started for SRT ");
    printing(&q);
    process *run_p = NULL;
    while (term != p_num)
    {
        // loop through all status
        // printf("time:%d\n",time);
        for (int m = preempt; m <= terminated; m++)
        {
            // loop through all process
            for (int i = 0; i < p_num; i++)
            {
                // check if process has special event at this time
                if (p_array[i].next == time && p_array[i].next_status == m)
                {

                    switch (m)
                    {
                    case arrived:

                        if (q.current_size != 0)
                        {
                            char c = q.queue[(q.current_size) - 1];
                            for (int j = 0; j < p_num; j++)
                            {
                                if (p_array[j].pid == c)
                                {
                                    p_array[i].next = p_array[j].next + p_array[j].cpu_time[p_array[j].cpu_index] + context_switch;
                                }
                            }
                        }
                        else
                        {
                            if (CPU_IN_USE)
                            {
                                p_array[i].next = cpu_end_time + context_switch / 2;
                            }
                            else
                            {
                                p_array[i].next = time;
                            }
                        }
                        p_array[i].next_status = c_switch_in;
                        adding_one(&q, p_array[i].pid);
                        if (q.current_size > 1)
                        {
                            queue_sort_tau(&q);

                            if (CPU_IN_USE)
                            {
                                sync_array(&q, cpu_end_time, context_switch, true);
                                // printf("time: %d\n",cpu_end_time);
                            }
                            else
                            {
                                sync_array(&q, time, context_switch, false);
                                // printf("time: %d\n",cpu_end_time);
                            }
                        }
                        printf("time %dms: Process %c (tau %dms) arrived; added to ready queue ", time, p_array[i].pid, p_array[i].tau);
                        printing(&q);
                        break;

                    case c_switch_in:

                        if (!CPU_IN_USE && q.queue[0] == p_array[i].pid)
                        {
                            deleting_first(&q);
                            // printf("%c, %d\n",p_array[i].pid, p_array[i].next);
                            p_array[i].next = time + context_switch / 2;
                            p_array[i].next_status = cpu_start;
                            if (p_array[i].remaining != 0)
                            {
                                cpu_end_time = p_array[i].next + p_array[i].remaining;
                            }
                            else
                            {
                                cpu_end_time = p_array[i].next + p_array[i].cpu_time[p_array[i].cpu_index];
                            }
                            run_p = &p_array[i];
                            CPU_IN_USE = true;
                        }
                        break;
                    case cpu_start:

                        if (p_array[i].remaining != 0)
                        {
                            p_array[i].next += p_array[i].remaining;
                            printf("time %dms: Process %c (tau %dms) started using the CPU for remaining %dms of %dms burst ", time, p_array[i].pid, p_array[i].tau, p_array[i].remaining, p_array[i].cpu_time[p_array[i].cpu_index]);
                            printing(&q);
                        }
                        else
                        {
                            // printf("%d\n",p_array[i].cpu_index);
                            p_array[i].next += p_array[i].cpu_time[p_array[i].cpu_index];
                            printf("time %dms: Process %c (tau %dms) started using the CPU for %dms burst ", time, p_array[i].pid, p_array[i].tau, p_array[i].cpu_time[p_array[i].cpu_index]);
                            printing(&q);
                        }
                        p_array[i].next_status = cpu_end;

                        process *c = compare(&(q.queue[0]), &(p_array[i].pid));
                        if (c->pid != p_array[i].pid)
                        {

                            printf("time %dms: Process %c (tau %dms) will preempt %c ", time, c->pid, c->tau, p_array[i].pid);
                            printing(&q);

                            p_array[i].next = time + context_switch / 2;
                            p_array[i].next_status = preempt;

                            c->next = time + context_switch / 2;
                            c->next_status = preempt;
                        }

                        break;

                    case cpu_end:

                        p_array[i].cpu_index++;
                        p_array[i].next += context_switch / 2;
                        if (p_array[i].total_burst == p_array[i].cpu_index)
                        {
                            printf("time %dms: Process %c terminated ", time, p_array[i].pid);
                            printing(&q);
                            term++;
                        }
                        else
                        {
                            printf("time %dms: Process %c (tau %dms) completed a CPU burst; %d bursts to go ", time, p_array[i].pid, p_array[i].tau, p_array[i].total_burst - p_array[i].cpu_index);
                            printing(&q);

                            int old_tau = p_array[i].tau;
                            p_array[i].tau = recalculate(alpha, p_array[i].tau, p_array[i].cpu_time[p_array[i].cpu_index - 1]);
                            p_array[i].remaining_tau = 0;

                            printf("time %dms: Recalculated tau for process %c: old tau %dms; new tau %dms ", time, p_array[i].pid, old_tau, p_array[i].tau);
                            printing(&q);

                            printf("time %dms: Process %c switching out of CPU; will block on I/O until time %dms ", time, p_array[i].pid, p_array[i].next + p_array[i].io_time[p_array[i].io_index]);
                            printing(&q);
                        }
                        p_array[i].next_status = c_switch_out;
                        // printf("running:%c %d %d\n",p_array[i].pid,p_array[i].next, p_array[i].next_status);
                        break;

                    case c_switch_out:

                        if (p_array[i].total_burst == p_array[i].cpu_index)
                        {
                            p_array[i].next_status = terminated;
                        }
                        else
                        {
                            p_array[i].next = time + p_array[i].io_time[p_array[i].io_index];
                            p_array[i].next_status = waiting;
                            p_array[i].io_index++;
                        }
                        run_p = NULL;
                        CPU_IN_USE = false;

                        break;
                    case waiting:
                        if (p_array[i].remaining != 0)
                        {
                            p_array[i].remaining = 0;
                        }

                        adding_one(&q, p_array[i].pid);
                        if (q.current_size > 1)
                        {
                            queue_sort_tau(&q);
                        }
                        if (CPU_IN_USE)
                        {
                            sync_array(&q, cpu_end_time, context_switch, true);
                            // printf("time: %d\n",cpu_end_time);
                        }
                        else
                        {
                            sync_array(&q, time, context_switch, false);
                            // printf("time: %d\n",cpu_end_time);
                        }

                        if (CPU_IN_USE)
                        {
                            int remaining = run_p->next - time;
                            int already = run_p->cpu_time[run_p->cpu_index] - remaining;
                            if (p_array[i].tau < (run_p->tau - already))
                            {
                                run_p->remaining = remaining;
                                run_p->next = time + context_switch / 2;
                                run_p->next_status = preempt;
                                run_p->remaining_tau = run_p->tau - already;
                                // current
                                p_array[i].next = time + context_switch / 2;
                                p_array[i].next_status = preempt;
                                printf("time %dms: Process %c (tau %dms) completed I/O; preempting %c ", time, p_array[i].pid, p_array[i].tau, run_p->pid);
                                printing(&q);
                            }
                            else
                            {
                                p_array[i].next_status = c_switch_in;
                                // print message
                                printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue ", time, p_array[i].pid, p_array[i].tau);
                                printing(&q);
                            }
                        }
                        else
                        {
                            p_array[i].next_status = c_switch_in;
                            // print message
                            printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue ", time, p_array[i].pid, p_array[i].tau);
                            printing(&q);
                        }

                        break;

                    case terminated:
                        break;

                        // preempting process
                        // delete first in queue
                        // add run_p to queue
                    case preempt:

                        printing(&q);
                        if (q.queue[0] == p_array[i].pid)
                        {

                            deleting_first(&q);
                            // printf("%c, %d\n",p_array[i].pid, p_array[i].next);
                            p_array[i].next = time + context_switch / 2;
                            p_array[i].next_status = cpu_start;
                            if (p_array[i].remaining != 0)
                            {
                                cpu_end_time = p_array[i].next + p_array[i].remaining;
                            }
                            else
                            {
                                cpu_end_time = p_array[i].next + p_array[i].cpu_time[p_array[i].cpu_index];
                            }
                            run_p = &p_array[i];
                            CPU_IN_USE = true;
                        }
                        else
                        {

                            printf("%c\n", p_array[i].pid);
                            p_array[i].next_status = c_switch_in;
                            adding_one(&q, p_array[i].pid);
                            if (q.current_size > 1)
                            {
                                queue_sort_tau(&q);
                            }
                            printf("%c\n", q.queue[0]);
                            sync_array(&q, cpu_end_time, context_switch, false);
                        }

                        break;
                    }
                }
            }
        }

        // smallest time
        int smallest_time = time;
        for (int i = 0; i < p_num; i++)
        {
            if (p_array[i].next_status != terminated && p_array[i].next > smallest_time)
            {
                smallest_time = p_array[i].next;
            }
        }

        for (int i = 0; i < p_num; i++)
        {
            if (p_array[i].next_status != terminated && p_array[i].next < smallest_time)
            {
                smallest_time = p_array[i].next;
                // printf("%c: %d %d\n",p_array[i].pid, p_array[i].next, p_array[i].next_status);
            }
        }

        time = smallest_time;
        // printf("%d\n",time);
    }
    // printf("time %dms: Simulator ended for SRT ", time);
    printing(&q);
}

void rr(int context_switch, int time_slice, FILE *fp)
{

    int time = 0;
    boolean CPU_IN_USE = false;
    int cpu_end_time = 0;
    queue q;
    queue_init(&q, p_num);
    int term = 0;
    
    //calculate total_cpu_time
    int total_cpu_use = 0;
    for (int i = 0; i < p_num; i++)
    {
        for (int j = 0; j < p_array[i].total_burst; j++)
        {
            total_cpu_use += p_array[i].cpu_time[j];
        }
    }
    //==========================================================

    int total_preempt_number = 0;
    int total_context_switch_number = 0;
    //==========================================================

    printf("time 0ms: Simulator started for RR with time slice %dms ", time_slice);
    printing(&q);
    while (term != p_num)
    {
        // loop through all status
        // printf("time:%d\n",time);
        for (int m = preempt; m < terminated; m++)
        {
            // loop through all process
            for (int i = 0; i < p_num; i++)
            {
                // check if process has special event at this time
                if ((p_array[i].next == time || p_array[i].time_expire == time) && p_array[i].next_status == m)
                {
                    switch (m)
                    {
                    case arrived:

                        if (q.current_size != 0)
                        {
                            char c = q.queue[(q.current_size) - 1];
                            for (int j = 0; j < p_num; j++)
                            {
                                if (p_array[j].pid == c)
                                {
                                    p_array[i].next = p_array[j].next + p_array[j].cpu_time[p_array[j].cpu_index] + context_switch;      
                                }
                            }
                        }
                        else
                        {
                            if (CPU_IN_USE)
                            {
                                p_array[i].next = cpu_end_time + context_switch / 2;
                            }
                            else
                            {
                                p_array[i].next = time;
                            }
                        }
                        
                        p_array[i].next_status = c_switch_in;
                        adding_one(&q, p_array[i].pid);
                        printf("time %dms: Process %c arrived; added to ready queue ", time, p_array[i].pid);
                        printing(&q);
                        break;

                    case c_switch_in:

                        if (!CPU_IN_USE && q.queue[0] == p_array[i].pid)
                        {
                            deleting_first(&q);
                            // printf("%c, %d\n",p_array[i].pid, p_array[i].next);
                            
                            p_array[i].next = time + context_switch / 2;
                            
                            p_array[i].next_status = cpu_start;
                            if (p_array[i].remaining != 0)
                            {
                                cpu_end_time = p_array[i].next + p_array[i].remaining;
                                total_context_switch_number++;
                            }
                            else
                            {
                                cpu_end_time = p_array[i].next + p_array[i].cpu_time[p_array[i].cpu_index];
                            }
                            CPU_IN_USE = true;
                        }
                        break;
                    case cpu_start:
                        if (p_array[i].remaining != 0)
                        {
                            p_array[i].next += p_array[i].remaining;
                            printf("time %dms: Process %c started using the CPU for remaining %dms of %dms burst ", time, p_array[i].pid, p_array[i].remaining, p_array[i].cpu_time[p_array[i].cpu_index]);
                            printing(&q);
                        }
                        else
                        {
                            // printf("%d\n",p_array[i].cpu_index);
                            p_array[i].next += p_array[i].cpu_time[p_array[i].cpu_index];
                            printf("time %dms: Process %c started using the CPU for %dms burst ", time, p_array[i].pid, p_array[i].cpu_time[p_array[i].cpu_index]);
                            printing(&q);
                        }
                        p_array[i].next_status = cpu_end;
                        p_array[i].time_expire = time + time_slice;

                        break;

                    case cpu_end:
                        if (p_array[i].next == time)
                        {
                            p_array[i].next_status = c_switch_out;
                            p_array[i].cpu_index++;
                            p_array[i].next += context_switch / 2;
                            if (p_array[i].total_burst == p_array[i].cpu_index)
                            {
                                printf("time %dms: Process %c terminated ", time, p_array[i].pid);
                                printing(&q);
                                term++;
                            }
                            else
                            {
                                if(p_array[i].total_burst - p_array[i].cpu_index == 1){
                                    printf("time %dms: Process %c completed a CPU burst; %d burst to go ", time, p_array[i].pid, p_array[i].total_burst - p_array[i].cpu_index);
                                    printing(&q);
                                }
                                else{
                                    printf("time %dms: Process %c completed a CPU burst; %d bursts to go ", time, p_array[i].pid, p_array[i].total_burst - p_array[i].cpu_index);
                                    printing(&q);
                                }

                                printf("time %dms: Process %c switching out of CPU; will block on I/O until time %dms ", time, p_array[i].pid, p_array[i].next + p_array[i].io_time[p_array[i].io_index]);
                                printing(&q);
                            }
                            p_array[i].time_expire = -1;
                        }
                        else
                        {
                            if (q.current_size == 0)
                            {
                                printf("time %dms: Time slice expired; no preemption because ready queue is empty ", time);
                                printing(&q);
                                p_array[i].time_expire += time_slice;
                            }
                            else
                            {
                                p_array[i].remaining = p_array[i].next - time;
                                printf("time %dms: Time slice expired; process %c preempted with %dms remaining ", time, p_array[i].pid, p_array[i].remaining);
                                printing(&q);
                                p_array[i].next = time + context_switch / 2;
                                p_array[i].next_status = preempt;
                                p_array[i].time_expire = -1;
                            }

                            // printf("running:%c %d %d\n",p_array[i].pid,p_array[i].next, p_array[i].next_status);
                        }
                        // printf("running expire %d :%c %d \n",time, p_array[i].pid,p_array[i].time_expire);

                        break;

                    case c_switch_out:

                        if (p_array[i].total_burst == p_array[i].cpu_index)
                        {
                            p_array[i].next_status = terminated;
                            total_context_switch_number++;
                        }
                        else
                        {
                            p_array[i].next = time + p_array[i].io_time[p_array[i].io_index];
                            p_array[i].next_status = waiting;
                            p_array[i].io_index++;
                            total_context_switch_number++;
                        }
                        CPU_IN_USE = false;

                        break;
                    case waiting:

                        if (p_array[i].remaining != 0)
                        {
                            p_array[i].remaining = 0;
                        }
                        // printf("%c: %d\n",p_array[i].pid, CPU_IN_USE);

                        if (q.current_size != 0)
                        {
                            char c = q.queue[(q.current_size) - 1];
                            for (int j = 0; j < p_num; j++)
                            {
                                if (p_array[j].pid == c)
                                {
                                    p_array[i].next = p_array[j].next + p_array[j].cpu_time[p_array[j].cpu_index] + context_switch;
                                    p_array[i].total_cpu_wait_t += p_array[i].next - time;
                                }
                            }
                        }
                        else
                        {
                            if (CPU_IN_USE)
                            {
                                p_array[i].next = cpu_end_time + context_switch / 2;
                                p_array[i].total_cpu_wait_t += p_array[i].next - time;
                                // printf("In_use: %c:%d\n", p_array[i].pid,p_array[i].next);
                            }
                            else
                            {
                                p_array[i].next = time;
                                p_array[i].total_cpu_wait_t += p_array[i].next - time;
                            }
                        }

                        adding_one(&q, p_array[i].pid);
                        p_array[i].next_status = c_switch_in;

                        // print message
                        printf("time %dms: Process %c completed I/O; added to ready queue ", time, p_array[i].pid);
                        printing(&q);

                        break;

                    case terminated:
                        break;

                    case preempt:
                        // printf("preempt:%c %d\n",p_array[i].pid, time);
                        total_preempt_number++;
                        CPU_IN_USE = false;
                        adding_one(&q, p_array[i].pid);
                        sync_array(&q, time, context_switch, false);
                        p_array[i].next_status = c_switch_in;

                        break;
                    }
                }
            }
        }
        // smallest time
        int smallest_time = time;
        for (int i = 0; i < p_num; i++)
        {
            if (p_array[i].next_status != terminated && p_array[i].next > smallest_time)
            {
                smallest_time = p_array[i].next;
            }
        }

        for (int i = 0; i < p_num; i++)
        {
            if (p_array[i].next_status != terminated)
            {
                if (p_array[i].next < smallest_time)
                {
                    smallest_time = p_array[i].next;
                    // printf("%c: %d\n",p_array[i].pid, p_array[i].next);
                }
                if ((p_array[i].time_expire != -1) && (p_array[i].time_expire < smallest_time))
                {
                    smallest_time = p_array[i].time_expire;
                    // printf("%c: %d\n",p_array[i].pid, p_array[i].time_expire);
                }
            }
        }

        time = smallest_time;
        // printf("%d\n",time);
    }
    total_context_switch_number++;
    printf("time %dms: Simulator ended for RR ", time);
    printing(&q);

    // Printing summary:
    double rr_avg_burst = get_avg_burst_time();
    double rr_avg_wait = get_avg_wait_time();
    //double rr_avg_tr = get_tr_time(accu_turn_around_time);

    double f_burst = total_cpu_use, f_time = time;
    double cpu_ult = f_burst / f_time * 100;
    cpu_ult = ceil(cpu_ult * 1000.0) / 1000.0;

    fprintf(fp, "Algorithm RR\n");
    fprintf(fp, "-- average CPU burst time: %.3f\n", rr_avg_burst);
    fprintf(fp, "-- average wait time: %.3f\n", rr_avg_wait);
    //fprintf(fp, "-- average turnaround time: %.3f\n", rr_avg_tr);
    fprintf(fp, "-- total number of context switches: %d\n", total_context_switch_number);
    fprintf(fp, "-- total number of preemptions: %d\n", total_preempt_number);
    fprintf(fp, "-- CPU utilization: %.3f%c\n", cpu_ult, 37);
    fprintf(fp, "\n");

    free(q.queue);
}

// comparator function:
process *compare(const void *c1, const void *c2)
{
    // getting the value of the address:
    char cc1 = *(const char *)c1;
    char cc2 = *(const char *)c2;
    // find the tau of the two char:
    process *p1;
    process *p2;
    int tau1, tau2 = 0;
    // finding tau1
    for (int i = 0; i < p_num; i++)
    {
        if (cc1 == p_array[i].pid)
        {
            p1 = &p_array[i];

            if (p_array[i].remaining_tau != 0)
            {
                tau1 = p_array[i].remaining_tau;
            }
            else
            {
                tau1 = p_array[i].tau;
            }
        }
        else if (cc2 == p_array[i].pid)
        {
            p2 = &p_array[i];
            if (p_array[i].remaining_tau != 0)
            {
                tau2 = p_array[i].remaining_tau;
            }
            else
            {
                tau2 = p_array[i].tau;
            }
        }
    }

    if (tau1 < tau2)
    {
        return p1;
    }
    else
    {
        return p2;
    }
}

// comparator function:
int comparator(const void *c1, const void *c2)
{
    // getting the value of the address:
    char cc1 = *(const char *)c1;
    char cc2 = *(const char *)c2;
    // find the tau of the two char:
    int tau1, tau2 = 0;
    // finding tau1
    for (int i = 0; i < p_num; i++)
    {
        if (cc1 == p_array[i].pid)
        {
            if (p_array[i].remaining_tau != 0)
            {
                tau1 = p_array[i].remaining_tau;
            }
            else
            {
                tau1 = p_array[i].tau;
            }
        }
    }

    // finding tau2
    for (int i = 0; i < p_num; i++)
    {
        if (cc2 == p_array[i].pid)
        {
            if (p_array[i].remaining_tau != 0)
            {
                tau2 = p_array[i].remaining_tau;
            }
            else
            {
                tau2 = p_array[i].tau;
            }
        }
    }

    if (tau1 != tau2)
    {
        if (tau1 < tau2)
        {
            // printf("0 :%d %d \n", tau1, tau2);
            return 0;
        }
        else
        {
            // printf("1 :%d %d \n", tau1, tau2);
            return 1;
        }
    }
    else
    {
        if (cc1 < cc2)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
}

void sync_array(queue *q, int cpu_time, int context_switch, boolean use)
{
    process prev = p_array[0];
    for (int j = 0; j < q->current_size; j++)
    {
        for (int i = 0; i < p_num; i++)
        {
            if (p_array[i].pid == q->queue[j])
            {
                if (j == 0)
                {
                    if (use)
                    {
                        p_array[i].next = cpu_time + context_switch / 2;
                    }
                    else
                    {
                        p_array[i].next = cpu_time;
                    }
                }
                else
                {
                    if (prev.remaining != 0)
                    {
                        p_array[i].next = prev.next + prev.remaining + context_switch;
                    }
                    else
                    {
                        p_array[i].next = prev.next + prev.cpu_time[prev.cpu_index] + context_switch;
                    }
                }

                prev = p_array[i];
            }
        }
    }
}

void queue_sort_tau(queue *q)
{
    // move everything into a temp char array, and sort it via tau, and put them back to the array:
    int queue_cur_size = q->current_size;
    char *temp_sort = calloc(queue_cur_size, sizeof(char));

    // put all the things into it:
    for (int i = 0; i < queue_cur_size; i++)
    {
        temp_sort[i] = q->queue[i];
    }

    qsort((char *)temp_sort, queue_cur_size, sizeof(char), comparator);

    // putting things back:
    // put all the things back into it:
    for (int i = 0; i < queue_cur_size; i++)
    {
        q->queue[i] = temp_sort[i];
    }

    free(temp_sort);
}