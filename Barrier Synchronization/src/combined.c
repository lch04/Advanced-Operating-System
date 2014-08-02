#include <stdio.h>
#include "mpi.h"
#include <stdlib.h>
#include <omp.h>
#include <pthread.h>

#define NUM_BARRIERS 1024
//#define NUM_OMP_THREADS 4

pthread_mutex_t thread_count_lock = PTHREAD_MUTEX_INITIALIZER;

int thread_count = 0;
int num_threads = 0;
int initialized = 0;
int global_sense = 1;
int test = 0;
int* local_sense = NULL;
int use_busy_wait = 0;
int busy_wait_count = 0;

void centralized_barrier_openmp(){
    int i,thread_id;
    while(!initialized){
        #pragma omp master
        {
            thread_count = omp_get_num_threads();
            num_threads = thread_count;
            local_sense = (int *)calloc(num_threads, sizeof(int));
            for(i=0;i<num_threads;i++)
                local_sense[i] = 1;
            #pragma omp critical    
            initialized = 1;
        }
    }
    thread_id = omp_get_thread_num();
    pthread_mutex_lock(&thread_count_lock);
    thread_count--;
    if(thread_count == 0){
        thread_count = num_threads;
        global_sense ^= 1;
        local_sense[thread_id] ^= 1;
        pthread_mutex_unlock(&thread_count_lock);
        return;
    }
    else{
        pthread_mutex_unlock(&thread_count_lock);
        while (local_sense[thread_id] == global_sense){
            if(use_busy_wait)
                for(i=0;i<busy_wait_count;i++);
            else
                ;
        }
        local_sense[thread_id] ^= 1;
        return;
    }
}

void centralized_barrier_mpi(int* rank, int* numprocs, int* sense) {
    int data = 1, i;

    MPI_Status status;

    if (*rank != (*numprocs)-1) {
        MPI_Send(&data, 1, MPI_INT, *numprocs-1, 1, MPI_COMM_WORLD);
	printf("Process %d finished sending data\n", *rank);
    }

    if (*rank == (*numprocs)-1) {
        for (i = 0 ; i < ((*numprocs)-1); ++i) {
            MPI_Recv(&data, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
        }
	printf("Process %d finished receiving data\n", *rank);
	*sense = !*sense;
    }

    MPI_Bcast(sense, 1, MPI_INT, *(numprocs)-1, MPI_COMM_WORLD);

    while (!*sense);
    *sense = !*sense;
}

int main(int argc, char **argv) {
    int rank, numprocs, sense = 0;
    double time, time1, time2;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    omp_set_dynamic(0);
    omp_set_num_threads(atoi(argv[1]));
    
    time1 = MPI_Wtime();
    #pragma omp parallel
    {
	int i = 0;
    for (i = 0; i < NUM_BARRIERS; ++i) {
        #pragma omp critical
        test++; 
        centralized_barrier_openmp();
        #pragma omp master
	printf("Test value is: %3d in Process %d\n",test, rank);
	centralized_barrier_openmp();
        #pragma omp master
        printf("Process %d entered barrier %d\n", rank, i);
        #pragma omp master
        centralized_barrier_mpi(&rank, &numprocs, &sense);
        centralized_barrier_openmp(); 
        #pragma omp master
        printf("Process %d left barrier %d\n", rank, i);
	centralized_barrier_openmp();
	#pragma omp master
	test = 0;
	centralized_barrier_openmp();
    }
    }
    time2 = MPI_Wtime();
    time += (time2 - time1);

    printf("Average time spent in barrier by process %d is %f\n", rank, time/NUM_BARRIERS);

    MPI_Finalize();
    return 0;
}


