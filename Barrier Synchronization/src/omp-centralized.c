#include <stdio.h>
#include <omp.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

// Timestamp calculation
#define TS(x) ((x.tv_sec*1000000)+x.tv_usec)

/* Locks for critical shared variables */
pthread_mutex_t thread_count_lock = PTHREAD_MUTEX_INITIALIZER;

/* Global shared data */
int thread_count = 0;
int num_threads = 0;
int initialized = 0;
int global_sense = 1;
int test = 0;
int *local_sense = NULL;

/* Command line arguments "<usage>: ./cntg <omp_thread_count> <num_barriers> <use_busy_wait> <busy_wait_count> */
int use_busy_wait =0;
int omp_thread_count =4;
int num_barriers = 1;
int busy_wait_count =100;

void barrier(){
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

void usage(){
	printf("Please check arguments!\n");
	printf("Usage: ./cntg <omp_thread_count> <num_barriers> <use_busy_wait> <busy_wait_count>\n\n");
	printf("omp_thread_count: Number of threads in the omp parallel region (<50)\n");
	printf("num_barriers:     Number of barriers to simulate (<1000)\n");
	printf("use_busy_wait:    Use busy wait while spinning to reduce memory contention (1 or 0)\n");
	printf("busy_wait_count:  Ceiling for the busy wait\n");
	exit(0);
}

int main(int argc, char* argv[]){
	if(argc!=5){
		usage();	
	}
	omp_set_dynamic(0);

	/* Parse Arguments */
	omp_thread_count = atoi(argv[1]);
	if(omp_thread_count > 50 || omp_thread_count <=1)
		usage();
	num_barriers = atoi(argv[2]);
	if(num_barriers > 1000 || num_barriers <1)
		usage();
	use_busy_wait = atoi(argv[3]);
	if(use_busy_wait!=0 && use_busy_wait!=1)
		usage();
	busy_wait_count = atoi(argv[4]);
	
	/* Run the experiment */
	struct timeval start,end;
	omp_set_num_threads(omp_thread_count);
	gettimeofday(&start, NULL);
	#pragma omp parallel
	{
		int i;
		for(i=0;i<num_barriers;i++){		
		#pragma omp critical
		test++;
		barrier();
		}
		#pragma omp master
		fprintf(stderr,"%3d\t",test);
	}
	gettimeofday(&end, NULL);
	fprintf(stderr,"%10.4f\tusec\n",(TS(end)-TS(start))/(double)num_barriers);
}
