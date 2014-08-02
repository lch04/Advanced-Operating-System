#include <stdio.h>
#include <omp.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

// Timestamp calculation
#define TS(x) ((x.tv_sec*1000000)+x.tv_usec)

/* Command line arguments "<usage>: ./mcs <omp_thread_count> <num_barriers> <arrival-m-ary> <wakeup-n-ary> <use_busy_wait> <busy_wait_count> */
int use_busy_wait =0;
int omp_thread_count =4;
int num_barriers = 1;
int busy_wait_count =100;
int ARRIVAL_M_ARY=4;
int WAKEUP_M_ARY=2;

//#define DEBUG 1

/* Global shared data */

typedef struct arrival_node_s{
	int thread_id;
	int *has_child;
	int *child_ready;
	int parent;
	int parent_index; 
} arrival_node_t;

typedef struct wakeup_node_s{
	int thread_id;
	int proceed;
	int *children;
} wakeup_node_t;

arrival_node_t *arrival_nodes;
wakeup_node_t *wakeup_nodes;

int num_threads = 0;
int remaining_threads;
int initialized = 0;
int test = 0;
int parent;
int parent_full=0;
arrival_node_t *nodes;


void initialize(){
/* Initialize arrival tree */
	int index,i,cur_parent,cur_parent_index;
	num_threads = omp_get_num_threads();
	remaining_threads = num_threads;
	parent = num_threads - remaining_threads;
	parent_full =0;

	arrival_nodes = (arrival_node_t *)malloc(num_threads*sizeof(arrival_node_t));
	/* Start iterating and populating the arrival tree structure */
	while(remaining_threads){
		index = num_threads - remaining_threads--; // Index will also be the thread ID
		
		arrival_nodes[index].has_child = (int *)malloc(ARRIVAL_M_ARY*sizeof(int));
		arrival_nodes[index].child_ready = (int *)malloc(ARRIVAL_M_ARY*sizeof(int));
		for(i=0;i<ARRIVAL_M_ARY;i++){
			arrival_nodes[index].has_child[i] = 0;
			arrival_nodes[index].child_ready[i] = 0;
		}		

		arrival_nodes[index].thread_id = index;
		/* Root Node */
		if (index == 0){
			arrival_nodes[index].parent = -1;
			arrival_nodes[index].parent_index = -1;
		}
		/* Child nodes */
		else{
			/* Set the parent for the current node */
			arrival_nodes[index].parent = parent;
			arrival_nodes[index].parent_index = parent_full;
			
			/* Set the has_child of the parent for cur_node */
			cur_parent = arrival_nodes[index].parent;
			cur_parent_index = arrival_nodes[index].parent_index;
			arrival_nodes[cur_parent].has_child[cur_parent_index] = 1;

			if(parent_full == ARRIVAL_M_ARY-1){
				parent_full = -1;
				parent++;
			}
			parent_full++;
		}	
	}
#ifdef DEBUG
	int j;
	fprintf(stderr,"\t\tArrival Tree\n");
	for(i=0;i<num_threads;i++){
		fprintf(stderr,"Node: %d\n\t",i);
		fprintf(stderr,"Thread_id: %d\n\t",arrival_nodes[i].thread_id);
		fprintf(stderr,"has_child: ");
		for(j=0;j<ARRIVAL_M_ARY;j++){
			fprintf(stderr,"%d",arrival_nodes[i].has_child[j]);
		}
		fprintf(stderr,"\n\t");
		fprintf(stderr,"child_ready: ");
		for(j=0;j<ARRIVAL_M_ARY;j++){
			fprintf(stderr,"%d",arrival_nodes[i].child_ready[j]);
		}
		fprintf(stderr,"\n\t");
		fprintf(stderr,"Parent: %d\n\t",arrival_nodes[i].parent);
		fprintf(stderr,"Parent index: %d\n",arrival_nodes[i].parent_index);
	}
//DEBUG_END
#endif

/* Initialize wakeup tree */
	remaining_threads = num_threads;
	int next_child = num_threads - remaining_threads;

	wakeup_nodes = (wakeup_node_t *)malloc(num_threads*sizeof(wakeup_node_t));
	/* Populate the wakeup Tree Structure */
	while(remaining_threads){
		index = num_threads - remaining_threads--;
		wakeup_nodes[index].thread_id = index;
		wakeup_nodes[index].proceed = 0;
		
		wakeup_nodes[index].children = (int *)malloc(WAKEUP_M_ARY*sizeof(int));
		for(i=0;i<WAKEUP_M_ARY;i++){
			if(++next_child < num_threads)
				wakeup_nodes[index].children[i] = next_child;
			else
				wakeup_nodes[index].children[i] = -1;
		}
	}
#ifdef DEBUG
//DEBUG
	fprintf(stderr,"\t\tWakeup Tree\n");
	for(i=0;i<num_threads;i++){
		fprintf(stderr,"Node: %d\n\t",i);
		fprintf(stderr,"Thread_id: %d\n\t",wakeup_nodes[i].thread_id);
		fprintf(stderr,"Proceed: %d\n\t",wakeup_nodes[i].proceed);
		fprintf(stderr,"Children: ");
		for(j=0;j<WAKEUP_M_ARY;j++){
			fprintf(stderr,"%d",wakeup_nodes[i].children[j]);
		}
		fprintf(stderr,"\n");
	}
//DEBUG_END
#endif
}

void wakeup(int thread_id){
	int i;
	if(thread_id != -1){
		//#pragma omp critical
		wakeup_nodes[thread_id].proceed = 1;
		for(i=0;i<ARRIVAL_M_ARY;i++)
			arrival_nodes[thread_id].child_ready[i] = 0;
		for(i=0;i<WAKEUP_M_ARY;i++){
			if(wakeup_nodes[thread_id].children[i]!=-1)
				wakeup(wakeup_nodes[thread_id].children[i]);
		}
	}
	return;
}

void set_ready(int thread_id, int parent, int parent_index){
	int i,j;
	if(parent == -1){//Root node reached in arrival tree, trigger wakeup
		for(i=0;i<ARRIVAL_M_ARY;i++){
			if (arrival_nodes[thread_id].has_child[i]){
				while(!arrival_nodes[thread_id].child_ready[i]){
				if(use_busy_wait)
					for(j=0;j<busy_wait_count;j++);
				else
					;
				}
			}
		}
		wakeup(parent+1);
		return;
	}
	else{
		for(i=0;i<ARRIVAL_M_ARY;i++){
			if (arrival_nodes[thread_id].has_child[i]){
				while(!arrival_nodes[thread_id].child_ready[i]){
				if(use_busy_wait)
					for(j=0;j<busy_wait_count;j++);
				else
					;
				}
			}
		}
		//#pragma omp critical
		arrival_nodes[parent].child_ready[parent_index] = 1;
		return;
	}
}

void barrier(){
	int thread_id,parent,parent_index;
	int i;
	while(!initialized){
		#pragma omp master
		{
			initialize();
			#pragma omp critical	
			initialized = 1;
		}
	}
	/* Get parent ID for cur_thread */
	thread_id = omp_get_thread_num();
	//printf("Thread %d entered barrier!\n",thread_id);
	parent = arrival_nodes[thread_id].parent;
	parent_index = arrival_nodes[thread_id].parent_index;
	
	/* Mark cur_thread as ready in the parent's data structure */
	set_ready(thread_id, parent, parent_index);

	/* Spin until Wakeup signal arrives */
	while(!wakeup_nodes[thread_id].proceed){
	if(use_busy_wait)
		for(i=0;i<busy_wait_count;i++);
	else
		;
	}
	/* Reset wakeup signalling variable */
	wakeup_nodes[thread_id].proceed = 0;
	//printf("Thread %d woken up!\n",thread_id);

	return;
}

usage(){
    printf("Please check arguments!\n");
    printf("Usage: ./mcs <omp_thread_count> <num_barriers> <arrival-m-ary> <wakeup-n-ary> <use_busy_wait> <busy_wait_count>\n\n");
    printf("omp_thread_count: Number of threads in the omp parallel region (<50)\n");
    printf("num_barriers:     Number of barriers to simulate (<1000)\n");
	printf("arrivary-m-ary:   M-ary arrival tree (4 to 8)\n");
	printf("wakeup-n-ary:     N-ary wakeup tree (2 to 6)\n");
    printf("use_busy_wait:    Use busy wait while spinning to reduce memory contention (1 or 0)\n");
    printf("busy_wait_count:  Ceiling for the busy wait\n");
    exit(0);
}


int main(int argc, char* argv[]){
	if(argc != 7)
		usage();

	omp_set_dynamic(0);
	/* Parse arguments */
	omp_thread_count = atoi(argv[1]);
    if(omp_thread_count > 50 || omp_thread_count <=1)
        usage();
    num_barriers = atoi(argv[2]);
    if(num_barriers > 1000 || num_barriers <1)
        usage();
	ARRIVAL_M_ARY = atoi(argv[3]);
	if(ARRIVAL_M_ARY > 8 || ARRIVAL_M_ARY < 4)
		usage();
	WAKEUP_M_ARY = atoi(argv[4]);
	if(WAKEUP_M_ARY > 6 || WAKEUP_M_ARY < 2)
		usage();
    use_busy_wait = atoi(argv[5]);
    if(use_busy_wait!=0 && use_busy_wait!=1)
        usage();
    busy_wait_count = atoi(argv[6]);

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
