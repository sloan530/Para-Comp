/* COMP 137-1 Spring 2018 February 21
// filename: pthread_hello.c
// A very minimal pthreads program (first example)
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* global variables are shared by all threads */

/* Number of threads to be created (in addition to the main thread) */
int thread_count = 4;

/* function that will be run by each thread */
void* Hello(void* rank);

/* main function - starting point for the main thread */

int main() {
    long thread;
    pthread_t* thread_handles;

    /* create an array of thread handles */
    thread_handles = malloc (thread_count*sizeof(pthread_t));

    /* start the threads, giving each a unique rank */
    for (thread = 0; thread < thread_count; thread++)
    {
        pthread_create(&thread_handles[thread], NULL,
          Hello, (void*) thread);
    }

    /* print a message from the main thread */
    printf("Hello from the main thread\n");

    /* wait for all threads to complete */
    for (thread = 0; thread < thread_count; thread++)
    {
        pthread_join(thread_handles[thread], NULL);
    }

   /* delete the array of thread handles */
   free(thread_handles);

   return 0;
}

/* function that will be run by each thread */

void* Hello(void* rank) {
    /* Parameters to thread functions arrive as void pointers.
       They must be cast to their true type. */
    long my_rank = (long) rank;

    /* Print something using a local variable (my_rank)
       and something from shared memory (thread_count) */
    printf("Hello from thread %ld of %d\n", my_rank, thread_count);

    /*  Nothing to return. Return a NULL pointer. */
    return NULL;
}
