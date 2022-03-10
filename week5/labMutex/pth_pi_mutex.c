/* COMP 137 Spring 2019
 * filename: pth_pi_busywait.c
 *
 * Purpose:  Estimate pi using the series
 *
 *              pi = 4*[1 - 1/3 + 1/5 - 1/7 + 1/9 - . . . ]
 *
 *           This version using busy-waiting to control access to
 *           the critical section.
 *
 * Run:      pth_pi_busywait <p> <n>
 *           p is the number of threads
 *           n is the number of terms of the Maclaurin series to use
 *           n should be evenly divisible by p
 *
 * IPP:   Section 4.5 (pp. 165 and ff.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

// VERSION == 1 : each term in series is added directly to shared sum
// VERSION == 2 : Each thread sums its terms locally, then adds result to shared sum
#define VERSION 2

/* shared/global data */
const int MAX_THREADS = 1024;
long thread_count;
long n;
int flag;
double sum;
float* timing_data;
float total_time;
//mutex
pthread_mutex_t mutex;
int ret;
/* read p and n from command lines and store in shared variables */
void processCommandLine(int argc, char* argv[]);
/* Each thread adds a specific number of terms from the series for
  computing PI. */
void* threadPiSum(void* rank);
/* get current clock time (seconds) */
float getTime();
/* write a spreadsheet (CSV) file containing the timing information */
void recordTimingData();

int main(int argc, char* argv[]) {
    long thread;
    pthread_t* thread_handles;
    float start, finish, elapsed;
    ret = pthread_mutex_init(&mutex, NULL);
    /* Get number of threads and number of terms from command line */
    processCommandLine(argc, argv);

    thread_handles = (pthread_t*) malloc (thread_count*sizeof(pthread_t));

    /* create shared array for threads to store their wait time */
    timing_data = malloc(thread_count*sizeof(float));

    start = getTime();
    sum = 0.0;
    flag = 0;
    for (thread = 0; thread < thread_count; thread++)
        pthread_create(&thread_handles[thread], NULL,
                       threadPiSum, (void*)thread);
    for (thread = 0; thread < thread_count; thread++)
        pthread_join(thread_handles[thread], NULL);
    finish = getTime();
    elapsed = finish - start;
    total_time = elapsed;

    sum = 4.0*sum;
    printf("With n = %ld terms,\n", n);
    printf("   Multi-threaded estimate of pi  = %.15f\n", sum);
    printf("   Elapsed time = %e seconds\n", elapsed);
    printf("   Math library estimate of pi    = %.15f\n",
           4.0*atan(1.0));

    recordTimingData();
    free(thread_handles);
    free(timing_data);
    ret = pthread_mutex_destroy(&mutex);
    return 0;
}

/* Each thread adds a specific number of terms from the series for
  computing PI. */
void* threadPiSum(void* rank) {
    long my_rank = (long) rank;
    double factor;
    long long i;
    long long my_n = n/thread_count;
    long long my_first_i = my_n*my_rank;
    long long my_last_i = my_first_i + my_n;
    float start, finish, elapsed;

    if (my_first_i % 2 == 0)
        factor = 1.0;
    else
        factor = -1.0;

    start = getTime();
#if VERSION==1
    for (i = my_first_i; i < my_last_i; i++, factor = -factor) {
        ret = pthread_mutex_lock(&mutex);
        sum += factor/(2*i+1);
        flag = (flag+1) % thread_count;
        ret = pthread_mutex_unlock(&mutex);
    }
#else
    float my_sum = 0.0;
    for (i = my_first_i; i < my_last_i; i++, factor = -factor) {
        my_sum += factor/(2*i+1);
    }
    ret = pthread_mutex_lock(&mutex);
    sum += my_sum;
    flag = (flag+1);
    ret = pthread_mutex_unlock(&mutex);
#endif
    finish = getTime();
    elapsed = finish - start;
    timing_data[my_rank] = elapsed;

    return NULL;
}

/* get current clock time (seconds) */
float getTime()
{
  struct timespec time_sec;
  double elapsed;
  clock_gettime(CLOCK_MONOTONIC, &time_sec);
  return time_sec.tv_nsec/1000000000.0;

}

/* print command line usage message and error message and abort program. */
void usage(char* prog_name) {
   fprintf(stderr, "usage: %s <p> <n>\n", prog_name);
   fprintf(stderr, "   p is the number of threads, 1 <= p < 1024\n");
   fprintf(stderr, "   n is the number of terms, n >= 1\n");
   fprintf(stderr, "   n should be evenly divisible by p\n");
   exit(0);
}

/* read p and n from command lines and store in shared variables */
void processCommandLine(int argc, char* argv[]) {
   if (argc != 3) usage(argv[0]);
   thread_count = strtol(argv[1], NULL, 10);
   if (thread_count <= 0 || thread_count > MAX_THREADS) usage(argv[0]);
   n = strtoll(argv[2], NULL, 10);
   if (n <= 0) usage(argv[0]);
}

/* write a spreadsheet (CSV) file containing the timing information */
void recordTimingData()
{
    short i;
#if VERSION==1
    FILE* fp = fopen("pth_pi_timing_1.csv", "w");
#else
    FILE* fp = fopen("pth_pi_timing_2.csv", "w");
#endif
    if (fp == NULL)
    {
        fprintf(stderr, "failed to open wait_time.csv\n");
        return;
    }
    fprintf(fp, "thread, processing time\n");
    for (i = 1; i<thread_count; i++)
    {
        fprintf(fp, "%d,%f\n", i, timing_data[i]);
    }
    fprintf(fp, "main,%f\n", total_time);
    fclose(fp);
}
