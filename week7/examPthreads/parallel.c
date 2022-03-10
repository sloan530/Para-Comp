/* File:      serial.c
 * Purpose:  Estimates the value of the natural logarithm of 2: ln(2)
 *
 *              ln(2) = 1 -1/2 + 1/3 - 1/4 ....
 *              nth term is (-1)^(n+1) 1/n
 *
 * Run:      parallel  <n> <p>
 *           n is the number of terms of the series to use
 *	     p is the number of threads
 *
 * Input:    none
 * Output:   The estimate of ln(2) and the value of ln(2) computed by the
 *           in the math library
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include "timer.h"

void Usage(char* prog_name);
void* ParallelWork(void* rank);

/*global variables*/
int p;
long n;
double sum;

//mutex
pthread_mutex_t mutex;

int main(int argc, char* argv[]) {
   	sum = 0.0;
	double start, finish;
	
   	if (argc != 3) Usage(argv[0]);
   	n = strtoll(argv[1], NULL, 10);
   	p = atoi(argv[2]);
   	
   	//printf("%ld %d\n", n, p);
   	
   	//pthread
    	pthread_t* thread_handles;
    	thread_handles = malloc(p*sizeof(pthread_t));
    	long thread;
    	
    	pthread_mutex_init(&mutex, NULL);
  
	GET_TIME(start);
	
	/* start the threads, giving each a unique rank */
    	for (thread = 0; thread < p; thread++)
    	{
        	pthread_create(&thread_handles[thread], NULL, ParallelWork, (void*) thread);
    	}

    	/* wait for all threads to complete */
    	for (thread = 0; thread < p; thread++)
    	{
        	pthread_join(thread_handles[thread], NULL);
    	}
	
	GET_TIME(finish);
	printf("Elapsed time = %e seconds\n", finish - start);


  	printf("   Program estimate of ln(2) = %.14f\n", sum);
   	printf("     From Math Library ln(2) = %.14f\n", log(2));
   
   	free(thread_handles);
   	pthread_mutex_destroy(&mutex);
   	
   	return 0;
}  /* main */

/*-----------------------------------------------------------------*/

void* ParallelWork(void* rank){
	long my_rank = (long) rank;
	double my_sign;
	double my_sum = 0.0;
	long i;
	long my_n = n/p;
	long first_i = my_n * my_rank;
	long last_i = first_i + my_n;
	
	//printf("%ld %ld - %ld\n", my_rank, first_i, last_i);
	
	if (first_i % 2 == 0) my_sign = -1.0;
	else my_sign = 1.0;
	if(first_i == 0) {
		first_i = 1;
		my_sign = -my_sign;
	}
	
	for(i = first_i; i < last_i; i++, my_sign = -my_sign){
		my_sum += my_sign*1/i;
	}
	
	pthread_mutex_lock(&mutex);
	sum += my_sum;
	pthread_mutex_unlock(&mutex);
	
	return NULL;
}

/*------------------------------------------------------------------
 * Function:  Usage
 * Purpose:   Print a message explaining how to run the program
 * In arg:    prog_name
 */
void Usage(char* prog_name) {
   fprintf(stderr, "usage: %s  <n> <p>\n", prog_name);  /* Change */
   fprintf(stderr, "   n is the number of terms and should be >= 1\n");
   fprintf(stderr, "   p is the number of threads and should be >=1\n");
   exit(0);
}  /* Usage */
