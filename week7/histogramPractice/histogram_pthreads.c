2   /* COMP 137 Spring 2019
 * filename: parallel_histogram_condvar_barrier.c
 *
 * Purpose:   Build a histogram from a list of random numbers
 *
 * Program arguments: ./histogram <bin_count> <min_meas> <max_meas> <data_count>
 *   <bin_count>  = number of bins in the histogram
 *   <min_meas>   = smallest possible value in list of random numbers
 *   <max_meas>   = largest possible value in list of random numbers
 *   <data_count> = number of values in list of random numbers
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include "timer.h"

/* GRAPHICAL_OUTPUT = 1 -> Show histogram with X's for number of
 *                         measurements in each bin
 * GRAPHICAL_OUTPUT != 1 -> Show histogram with text values for number of
 *                          measurements in each bin
 */
#define GRAPHICAL_OUTPUT 0

/* VERBOSE = 1 -> show extra debugging output
 * VERBOSE != 1 -> do not show extra debugging output
 */
#define VERBOSE 0

void usage(char prog_name[]);

void extractCommandLineArgs(
    int argc               /* in */,
    char*    argv[]        /* in  */,
    int*     bin_count_p   /* out */,
    float*   min_meas_p    /* out */,
    float*   max_meas_p    /* out */,
    int*     data_count_p  /* out */,
    int*     num_threads_p /* out */);

void generateData(
    float   min_meas    /* in  */,
    float   max_meas    /* in  */,
    float   data[]      /* out */,
    int     data_count  /* in  */);

void createBins(
    float min_meas      /* in  */,
    float max_meas      /* in  */,
    float bin_maxes[]   /* out */,
    int   bin_counts[]  /* out */,
    int   bin_count     /* in  */);

int findBin(
    float    data         /* in */,
    float    bin_maxes[]  /* in */,
    int      bin_count    /* in */,
    float    min_meas     /* in */);

void printHistogram(
    float    bin_maxes[]   /* in */,
    int      bin_counts[]  /* in */,
    int      bin_count     /* in */,
    float    min_meas      /* in */);

void* threadWork(void* args);

int** local_bin_counts;
int* bin_counts;
int num_threads;

/* barrier control */
int barrier_thread_count = 0;
pthread_mutex_t barrier_mutex;
pthread_cond_t ok_to_proceed;

typedef struct {
    long    rank;        /* the thread's unique rand/id */
    long    num_threads; /* number of threads */
    float*  data;        /* full array of data */
    long    data_count;  /* number of values in data array */
    float*  bin_maxes;   /* maximum value for each bin */
    int     bin_count;   /* number of bins */
    float   min_meas;    /* smallest possible value (lowest value for first bin) */
} THREAD_ARG;

int main(int argc, char* argv[])
{
    int bin_count, bin_sum;
    float min_meas, max_meas;
    float* bin_maxes;

    int data_count;
    float* data;
    long t, bin;
    pthread_t* thread_handles;
    THREAD_ARG* thread_arguments;

    double setup_time, thread_time, print_time;
    double t1, t2;

    GET_TIME(t1);

    /* Check and get command line args */
    extractCommandLineArgs(argc, argv, &bin_count, &min_meas, &max_meas, &data_count, &num_threads);

    /* Allocate arrays needed */
    bin_maxes = malloc(bin_count*sizeof(float));
    bin_counts = malloc(bin_count*sizeof(int));
    data = malloc(data_count*sizeof(float));

    local_bin_counts = calloc(num_threads,sizeof(int*));
    for (t=0; t<num_threads; t++)
        local_bin_counts[t] = calloc(bin_count,sizeof(int));

    /* Generate the data */
    generateData(min_meas, max_meas, data, data_count);

    /* START PARALLELIZATION */

    /* Create bins for storing counts */
    createBins(min_meas, max_meas, bin_maxes, bin_counts, bin_count);

    GET_TIME(t2);
    setup_time = t2-t1;
    t1 = t2;

    barrier_thread_count = 0;
    pthread_mutex_init(&barrier_mutex, NULL);
    pthread_cond_init(&ok_to_proceed, NULL);

    /* allocate thread handles */
    thread_handles = (pthread_t*)malloc(num_threads*sizeof(pthread_t));
    /* allocate thread argument structures */
    thread_arguments = (THREAD_ARG*)malloc(num_threads*sizeof(THREAD_ARG));

    /* create the threads, give each a unique rank and a random number */
    for (t = 0; t < num_threads; t++)
    {
        thread_arguments[t].rank = t;
        thread_arguments[t].num_threads = num_threads;
        thread_arguments[t].data = data;
        thread_arguments[t].data_count = data_count;
        thread_arguments[t].bin_maxes = bin_maxes;
        thread_arguments[t].bin_count = bin_count;
        thread_arguments[t].min_meas = min_meas;

        pthread_create(&thread_handles[t],
                       NULL,
                       threadWork,
                       (void*) &(thread_arguments[t])
                       );
    }

    /* Count number of values in each bin */
    /*
    for (i = 0; i < data_count; i++)
    {
        bin = findBin(data[i], bin_maxes, bin_count, min_meas);
        bin_counts[bin]++;
    }
    */

    /* wait for all threads to finish */
    for (t = 0; t < num_threads; t++)
        pthread_join(thread_handles[t], NULL);

    /* END PARALLELIZATION */
    GET_TIME(t2);
    thread_time = t2-t1;
    t1 = t2;

    /* Print the histogram */
    printHistogram(bin_maxes, bin_counts, bin_count, min_meas);

    GET_TIME(t2);
    print_time = t2-t1;
    t1 = t2;

    /* sum all bin counts to check answer */
    bin_sum = 0;
    for (bin=0; bin<bin_count; bin++)
        bin_sum += bin_counts[bin];
    printf("bin sum = %d\n", bin_sum);

    printf("setup time = %f\n", setup_time);
    printf("thread time = %f\n", thread_time);
    printf("print time = %f\n", print_time);

    pthread_cond_destroy(&ok_to_proceed);
    pthread_mutex_destroy(&barrier_mutex);

    free(data);
    free(bin_maxes);
    free(bin_counts);
    return 0;
}

/*---------------------------------------------------------------------
 * Function:  threadWork
 * Purpose:   Define work for a thread.
 * In arg:    args:  pointer to function's argument structure
 */
 void* threadWork(void* args) {
    long    rank = ((THREAD_ARG*)args)->rank;
    long    num_threads = ((THREAD_ARG*)args)->num_threads;
    float*  data = ((THREAD_ARG*)args)->data;
    long    data_count = ((THREAD_ARG*)args)->data_count;
    float*  bin_maxes = ((THREAD_ARG*)args)->bin_maxes;
    int     bin_count = ((THREAD_ARG*)args)->bin_count;
    float   min_meas = ((THREAD_ARG*)args)->min_meas;

    int i, bin, t;
    int n = data_count / num_threads;
    int start = n*rank;
    int end = start + n;
    if (end > data_count) end = data_count;

    /* Count number of values in each bin */
    for (i = start; i < end; i++)
    {
        bin = findBin(data[i], bin_maxes, bin_count, min_meas);
        local_bin_counts[rank][bin]++;
    }

    /*........... barrier ..........*/
    printf("thread %ld entering barrier\n", rank);
    /* wait for the barrier mutex */
    pthread_mutex_lock(&barrier_mutex);
    /* increment counter to indicate thread's arrival */
    barrier_thread_count++;
    /* if not last thread to arrive */
    if (barrier_thread_count < num_threads)
    {
        /* wait unlocks barrier_mutex and puts thread to sleep */
        /* wait is in a loop in case some other event wakes the thread */
        while (pthread_cond_wait(&ok_to_proceed, &barrier_mutex) != 0);
        /* barrier_mutex is relocked when pthread_cond_wait returns */
    }
    else {
        /* clear thread counter */
        barrier_thread_count = 0;
        /* signal all threads to wake up */
        pthread_cond_broadcast(&ok_to_proceed);
    }
    /* release barrier mutex */
    pthread_mutex_unlock(&barrier_mutex);
    printf("thread %ld leaving barrier\n", rank);

    if (rank == 0)
    {
        /* sum values from local bin counts */
        for (bin=0; bin<bin_count; bin++)
        {
            bin_counts[bin] = 0;
            for (t=0; t<num_threads; t++)
                bin_counts[bin] += local_bin_counts[t][bin];
        }
        printf("thread %ld has finished accumulating results\n", rank);
    }

    return NULL;
 }

/*---------------------------------------------------------------------
 * Function:  usage
 * Purpose:   Print a message showing how to run program and quit
 * In arg:    prog_name:  the name of the program from the command line
 */
void usage(char prog_name[] /* in */)
{
    fprintf(stderr, "usage: %s ", prog_name);
    fprintf(stderr, "<bin_count> <min_meas> <max_meas> <data_count> <num_threads>\n");
    exit(0);
}  /* Usage */


/*---------------------------------------------------------------------
 * Function:  extractCommandLineArgs
 * Purpose:   Get the command line arguments
 * In arg:    argv:  strings from command line
 * Out args:  bin_count_p:   number of bins
 *            min_meas_p:    minimum measurement
 *            max_meas_p:    maximum measurement
 *            data_count_p:  number of measurements
 */
void extractCommandLineArgs(
    int argc               /* in */,
    char*    argv[]        /* in  */,
    int*     bin_count_p   /* out */,
    float*   min_meas_p    /* out */,
    float*   max_meas_p    /* out */,
    int*     data_count_p  /* out */,
    int*     num_threads_p /* out */)
{
    if (argc != 6)
        usage(argv[0]);
    *bin_count_p = strtol(argv[1], NULL, 10);
    *min_meas_p = strtof(argv[2], NULL);
    *max_meas_p = strtof(argv[3], NULL);
    *data_count_p = strtol(argv[4], NULL, 10);
    *num_threads_p = strtol(argv[5], NULL, 10);
#if VERBOSE == 1
    printf("bin_count = %d\n", *bin_count_p);
    printf("min_meas = %f, max_meas = %f\n", *min_meas_p, *max_meas_p);
    printf("data_count = %d\n", *data_count_p);
#endif
}


/*---------------------------------------------------------------------
 * Function:  generateData
 * Purpose:   Generate random floats in the range min_meas <= x < max_meas
 * In args:   min_meas:    the minimum possible value for the data
 *            max_meas:    the maximum possible value for the data
 *            data_count:  the number of measurements
 * Out arg:   data:        the actual measurements
 */
void generateData(
    float   min_meas    /* in  */,
    float   max_meas    /* in  */,
    float   data[]      /* out */,
    int     data_count  /* in  */)
{
    int i;

    srand(0);
    for (i = 0; i < data_count; i++)
    {
        data[i] = min_meas + (max_meas - min_meas)*rand()/((double) RAND_MAX);
        if (data[i] == max_meas)
            data[i]--;
    }

#if VERBOSE == 1
    printf("data = ");
    for (i = 0; i < data_count; i++)
        printf("%4.3f ", data[i]);
    printf("\n");
#endif
}


/*---------------------------------------------------------------------
 * Function:  createBins
 * Purpose:   Compute max value for each bin, and store 0 as the
 *            number of values in each bin
 * In args:   min_meas:   the minimum possible measurement
 *            max_meas:   the maximum possible measurement
 *            bin_count:  the number of bins
 * Out args:  bin_maxes:  the maximum possible value for each bin
 *            bin_counts: the number of data values in each bin
 */
void createBins(
    float min_meas      /* in  */,
    float max_meas      /* in  */,
    float bin_maxes[]   /* out */,
    int   bin_counts[]  /* out */,
    int   bin_count     /* in  */)
{
    float bin_width;
    int   i;

    bin_width = (max_meas - min_meas)/bin_count;

    for (i = 0; i < bin_count; i++)
    {
        bin_maxes[i] = min_meas + (i+1)*bin_width;
        bin_counts[i] = 0;
    }

#if VERBOSE == 1
    printf("bin_maxes = ");
    for (i = 0; i < bin_count; i++)
        printf("%4.3f ", bin_maxes[i]);
    printf("\n");
#endif
}


/*---------------------------------------------------------------------
 * Function:  findBin
 * Purpose:   Use binary search to determine which bin a measurement
 *            belongs to
 * In args:   data:       the current measurement
 *            bin_maxes:  list of max bin values
 *            bin_count:  number of bins
 *            min_meas:   the minimum possible measurement
 * Return:    the number of the bin to which data belongs
 * Notes:
 * 1.  The bin to which data belongs satisfies
 *
 *            bin_maxes[i-1] <= data < bin_maxes[i]
 *
 *     where, bin_maxes[-1] = min_meas
 * 2.  If the search fails, the function prints a message and exits
 */
int findBin(
    float   data          /* in */,
    float   bin_maxes[]   /* in */,
    int     bin_count     /* in */,
    float   min_meas      /* in */)
{
    int bottom = 0, top =  bin_count-1;
    int mid;
    float bin_max, bin_min;

    while (bottom <= top)
    {
        mid = (bottom + top)/2;
        bin_max = bin_maxes[mid];
        bin_min = (mid == 0) ? min_meas: bin_maxes[mid-1];
        if (data >= bin_max)
            bottom = mid+1;
        else if (data < bin_min)
            top = mid-1;
        else
            return mid;
    }

    /* Whoops! (this should not happen)*/
    fprintf(stderr, "Data = %f doesn't belong to a bin!\n", data);
    fprintf(stderr, "Quitting\n");
    exit(-1);
}


/*---------------------------------------------------------------------
 * Function:  printHistogram
 * Purpose:   Print a histogram. Format of histogram is
 *            determined by value of GRAPHICAL_OUTPUT
 * In args:   bin_maxes:   the max value for each bin
 *            bin_counts:  the number of elements in each bin
 *            bin_count:   the number of bins
 *            min_meas:    the minimum possible measurement
 */
void printHistogram(
    float  bin_maxes[]   /* in */,
    int    bin_counts[]  /* in */,
    int    bin_count     /* in */,
    float  min_meas      /* in */)
{
    int i;
    float bin_max, bin_min;

    for (i = 0; i < bin_count; i++)
    {
        bin_max = bin_maxes[i];
        bin_min = (i == 0) ? min_meas: bin_maxes[i-1];
        printf("%.3f-%.3f:\t", bin_min, bin_max);
#if GRAPHICAL_OUTPUT == 1
        int j;
        for (j = 0; j < bin_counts[i]; j++)
            printf("X");
#else
        printf("%d", bin_counts[i]);
#endif
        printf("\n");
    }
}
