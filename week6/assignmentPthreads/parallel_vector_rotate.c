/* serial_vector_rotate.c
 * COMP 137 - OpenMPHW
 *
 * Program functionality:
 * Read 3D rotations and 3D vectors from a text file.
 * Build a rotation matrix and rotate all vectors using the rotation matrix.
 * Sum the rotated vectors.
 *
 * Program has a single command line argument, which is the
 * name of the data file that should be read and processed.
 *
 *  parallelize this program using pthreads.
 *
 * result for input1.txt:
 * Result = [-613.67, 28.55, -162.24]
 * result for input2.txt:
 * Result = [-661.51, -234.53, -179.22]
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include "timer.h"
#include <semaphore.h>


/* global variables */
char* input_file_name = NULL;
int num_threads;
long num_vectors = 0;
float* original_vectors = NULL;
float* rotated_vectors = NULL;

//mutex
pthread_mutex_t mutex;
int ret;

//semaphore barrier
int counter;
sem_t count_sem;
sem_t barrier_sem;


/*--------------------------------------------------------------------*/

void* parallelWork(void* args);
void processCommandLine(int argc, char* argv[]);
float* readInputDatafile(char* filename, long* num_vects, float angles[3]);
void multMatrixMatrix(float a[9], float b[9], float c[9]);
void multMatrixVector(float a[9], float b[3], float c[3]);
void addVectorVector(float a[3], float b[3], float c[3]);
void computeRotationMatrix(float angles[3], float rotation_matrix[9]);

/*--------------------------------------------------------------------*/

typedef struct {
    long  rank;        
    float* rotation_matrix;
    float* result;
} THREAD_ARG;

/*--------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
	
    	/* allocate local variables */
    	long v;
    	float rotation_matrix[9];
    	float angles[3];
    	float result[3] = { 0.0f, 0.0f, 0.0f };
    	float temp[3] = { 0.0f, 0.0f, 0.0f };
    	
    	//pthread
    	long thread;
    	pthread_t* thread_handles;
    	THREAD_ARG* thread_arguments;
    	
    	//mutex
    	ret = pthread_mutex_init(&mutex, NULL);
    	
    	//time
    	double start, finish;
	
    	/* check for command line argument */
	processCommandLine(argc, argv);
	
	/*create an array of thread handles*/
	thread_handles = malloc(num_threads*sizeof(pthread_t));
	thread_arguments = (THREAD_ARG*)malloc(num_threads*sizeof(THREAD_ARG));
	
    	/* read the file specified in the command line argument
           the reader function allocates the space for the input vectors */
    	original_vectors = readInputDatafile(input_file_name, &num_vectors, angles);
    	if (original_vectors == NULL)
    	{
        	fprintf(stderr, "could not read input file %s\n", input_file_name);
		exit(0);
    	}
	
	
    	/* allocated space for rotated vectors
           and compute the rotation transformation matrix */
    	rotated_vectors = (float*)malloc(3*num_vectors*sizeof(float));
    	computeRotationMatrix(angles, rotation_matrix);

	//initialize semaphore barrier control
	counter = 0;
	sem_init(&barrier_sem, 0, 0);
   	sem_init(&count_sem, 0, 1);
	
	
	GET_TIME(start);
	
	/* parallelWork: start the threads, giving each a unique rank */
	for(thread = 0; thread<num_threads; thread++){
		thread_arguments[thread].rank = thread;
		thread_arguments[thread].rotation_matrix = rotation_matrix;
        	//thread_arguments[thread].v = v;
        	thread_arguments[thread].result = result;
        	
		pthread_create(
			&thread_handles[thread], 
			NULL, 
			parallelWork, 
			(void*) &thread_arguments[thread]
			);
			
	}
	
	/* wait for all threads to complete */
	for(thread = 0; thread<num_threads; thread++){
		pthread_join(thread_handles[thread], NULL);
	}
	
	GET_TIME(finish);
	printf("Elapsed time = %e seconds\n", finish - start);
	
	//destroy semaphore barrier
	sem_destroy(&count_sem);
   	sem_destroy(&barrier_sem);
    	

    	/* print results */
    	printf("Result = [%0.2f, %0.2f, %0.2f]\n", result[0], result[1], result[2]);

    	/* clean up dynamic memory */
    	free(original_vectors);
    	free(rotated_vectors);
	free(thread_handles);
	ret = pthread_mutex_destroy(&mutex);

    	return 0;
}

/*--------------------------------------------------------------------*/

void* parallelWork(void* args){
	long my_rank = ((THREAD_ARG*)args)->rank;
    	float* rotation_matrix = ((THREAD_ARG*)args)->rotation_matrix;
    	long v = 0;
    	float* result = ((THREAD_ARG*)args)->result;
	float my_result[3] = { 0.0f, 0.0f, 0.0f };
    	float temp[3] = { 0.0f, 0.0f, 0.0f };
    	
    	
	int first_i = (num_vectors/num_threads)*my_rank;
	int last_i = (num_vectors/num_threads)*(my_rank+1);
	if (last_i > num_vectors) last_i = num_vectors;
	
	
	for (v=first_i; v<last_i; v++){
		multMatrixVector(
			rotation_matrix, 
			&(original_vectors[v*3]), 
			&(rotated_vectors[v*3])
			);
	}
	
        //barrier
        sem_wait(&count_sem);
	if(counter < num_threads-1){
		counter ++;
		sem_post(&count_sem);
		sem_wait(&barrier_sem);
	}
	else{
		counter = 0;
		sem_post(&count_sem);
		for(int j=0; j<num_threads-1; j++){
			sem_post(&barrier_sem);
		}
	}
	
        for(v=first_i; v<last_i; v++){
		
		addVectorVector(my_result, &(rotated_vectors[v*3]), temp);
		my_result[0] = temp[0];
        	my_result[1] = temp[1];
        	my_result[2] = temp[2];
        	
    		
	}
	
	//critical section: lock
	
	pthread_mutex_lock(&mutex);
    	addVectorVector(result, my_result, temp);
    	result[0] = temp[0];
    	result[1] = temp[1];
    	result[2] = temp[2];
    	pthread_mutex_unlock(&mutex);
    	
    	//ret = pthread_mutex_lock(&mutex);
	//result[0] = my_result[0];

        //result[1] = my_result[1];
        //result[2] = my_result[2];
	//ret = pthread_mutex_unlock(&mutex);
   	
	return NULL;

}


/* print command line usage message and abort program. */
void usage(char* prog_name) {
	fprintf(stderr, "usage: %s <inputFile> <# of threads>\n", prog_name);
	fprintf(stderr, "   <fn> is name of the file containing the data to be processed\n");
	exit(0);
}

/* interpret command lines and store in shared variables */
void processCommandLine(int argc, char* argv[]) {
	if (argc!=3) usage(argv[0]);

	input_file_name = argv[1];
	num_threads = atoi(argv[2]);
}

/* read the input data file */
float* readInputDatafile(char* filename, long* num_vects, float angles[3])
{
	long i = 0, j = 0, n = 0;
	float* input_vectors;

	FILE* fp = fopen(filename, "r");
	if (fp == NULL) return NULL;
	fscanf(fp, "%f, %f, %f\n", &(angles[0]), &(angles[1]), &(angles[2]));
	fscanf(fp, "%ld\n", &n);
	*num_vects = n;
	input_vectors = (float*)malloc(3 * n * sizeof(float));
	for (i = 0; i<n; i++)
	{
		fscanf(fp, "%f, %f, %f\n", &(input_vectors[j]), &(input_vectors[j + 1]), &(input_vectors[j + 2]));
		j += 3;
	}
	fclose(fp);
	return input_vectors;
}

/*--------------------------------------------------------------------*/
/*
 * Matrix and vector mathematics
 * These functions are thread safe.
*/

void multMatrixMatrix(float a[9], float b[9], float c[9])
{
	/* c = a*b */
	c[0] = a[0] * b[0] + a[1] * b[3] + a[2] * b[6];
	c[1] = a[0] * b[1] + a[1] * b[4] + a[2] * b[7];
	c[2] = a[0] * b[2] + a[1] * b[5] + a[2] * b[8];
	c[3] = a[3] * b[0] + a[4] * b[3] + a[5] * b[6];
	c[4] = a[3] * b[1] + a[4] * b[4] + a[5] * b[7];
	c[5] = a[3] * b[2] + a[4] * b[5] + a[5] * b[8];
	c[6] = a[6] * b[0] + a[7] * b[3] + a[7] * b[6];
	c[7] = a[6] * b[1] + a[7] * b[4] + a[7] * b[7];
	c[8] = a[6] * b[2] + a[7] * b[5] + a[7] * b[8];
}

void multMatrixVector(float a[9], float b[3], float c[3])
{
	/* c = a*b */
	c[0] = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	c[1] = a[3] * b[0] + a[4] * b[1] + a[5] * b[2];
	c[2] = a[6] * b[0] + a[7] * b[1] + a[8] * b[2];
}

void addVectorVector(float a[3], float b[3], float c[3])
{
	/* c = a + b */
	c[0] = a[0] + b[0];
	c[1] = a[1] + b[1];
	c[2] = a[2] + b[2];
}

void computeRotationMatrix(float angles[3], float rotation_matrix[9])
{
	float r = angles[2]; /* roll (radians) */
	float p = angles[0]; /* pitch (radians) */
	float y = angles[1]; /* yaw (radians) */
	float rx[9] =
	{ 1.0f,       0.0f,       0.0f,
		0.0f,       cosf(p),     -sinf(p),
		0.0f,       sinf(p),     cosf(p)
	};
	float ry[9] =
	{ cosf(y),    0.0f,       sinf(y),
		0.0f,       1.0f,       0.0f,
		-sinf(y),   0.0f,       cosf(y)
	};
	float rz[9] =
	{ cosf(r),    -sinf(r),   0.0f,
		sinf(r),    cosf(r),    0.0f,
		0.0f,       0.0f,       1.0f
	};
	float ry_rx[9];
	multMatrixMatrix(ry, rx, ry_rx);
	multMatrixMatrix(ry_rx, rz, rotation_matrix);
}
