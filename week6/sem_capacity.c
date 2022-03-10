/* COMP 137 Spring 2019
 * filename: sem_capacity.c
 *
 * This program starts threads with various processing loads.
 * The number of active threads is controlled by a semaphore.
 * Program reports process time at which a thread begins waiting,
 * starts processing and completes processing.
 *
 * Program is configured by the following variables:
 *  num_threads = total number of threads
 *  max_active_threads = number of threads that can be
 *                       actively processing at any time
 *  randomize_workload == 0 -> threads are all give same size tasks
 *  randomize_workload != 0 -> threads are give different sized tasks
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "timer.h"

/* configuration variables */
int num_threads = 10;
int max_active_threads = 3;
int randomize_workload = 0;

/* thread control */
sem_t count_sem;
void* threadWork(void* args);
int doSomethingToConsumeTime(long rank);
float *task_size;

/* timing */
double start, finish;
double pTime() { GET_TIME(finish); return finish-start; }

int main(int argc, char* argv[])
{
    int t;
    pthread_t* thread_handles;

    /* initialize timer */
    GET_TIME(start);

    /* set processing loads for threads */
    task_size = malloc(num_threads*sizeof(float));
    for (t=0; t<num_threads; t++)
        if (randomize_workload)
            task_size[t] = 1.0+(((rand()%200)-100)/200.0);
        else
            task_size[t] = 1.0;

    /* initialize the semaphore */
    sem_init(&count_sem, 0, max_active_threads);

    /* allocate thread handles */
    thread_handles = (pthread_t*)malloc(num_threads*sizeof(pthread_t));

    /* create the threads, give each a unique rank */
    for (t = 0; t < num_threads; t++)
        pthread_create(&thread_handles[t], NULL,
                       threadWork, (void*)t);

    /* wait for all threads to finish */
    for (t = 0; t < num_threads; t++)
        pthread_join(thread_handles[t], NULL);

    /* clean up */
    sem_destroy(&count_sem);
    free(thread_handles);
    return 0;
}

/*---------------------------------------------------------------------
 * Function:  threadWork
 * Purpose:   Define work for a thread.
 * In arg:    args:  pointer to function's argument structure
 */
void* threadWork(void* args)
{
    long rank = (long)args;

    printf("thread %ld waiting %f\n", rank, pTime());

    /* wait for semaphore */
    sem_wait(&count_sem);

    printf("thread %ld running %f\n", rank, pTime());

    doSomethingToConsumeTime(rank);

    printf("thread %ld leaving %f\n", rank, pTime());

    /* release the semaphore */
    sem_post(&count_sem);

    return NULL;
}

/* useless function to consume processing time */
int doSomethingToConsumeTime(long rank)
{
    int workload = 10000*task_size[rank];
    int x=0, i, j;
    for (i=0; i<workload; i++)
        for (j=0; j<100000; j++)
            x += i*j;
    return x;
}
