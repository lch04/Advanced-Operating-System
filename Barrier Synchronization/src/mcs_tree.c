/*
 * mcs_tree.c
 *
 *  Created on: Feb 21, 2014
 *      Author: chenhao
 */

#include "mcs_tree.h"

#define NUM_BARRIERS 1024 


void init(node* n, int* rank, int* numprocs) {
	int i;
	for (i = 1; i <= *numprocs; ++i) {
		n->have_child[i-1] = (4*(*rank)+i < *numprocs) ? (4*(*rank)+i):0;
		n->arrival_parent = (*rank == 0) ? DUMMY:floor((*rank-1)/4);
		n->wakeup_parent = (*rank == 0) ? DUMMY:((*rank)%2 == 0 ? (*rank-2)/2 : (*rank-1)/2);
		n->child_ptr[0] = (2*(*rank)+1 >= *numprocs) ? DUMMY:(2*(*rank)+1);
		n->child_ptr[1] = (2*(*rank)+2 >= *numprocs) ? DUMMY:(2*(*rank)+2);
	}
}

void print_tree(node* n, int* rank) {
	int i,j;
	printf("Process %d is printing tree\n", *rank);
	printf("Process %d's have_child array is ", *rank);
	for (i = 0; i < 4; ++i) {
	    printf("%d\t", n->have_child[i]);
	}
	printf("\nProcess %d's child_ptr array is ", *rank);
	for (j = 0; j < 2; ++j) {
	    printf("%d\t", n->child_ptr[j]);
	} 
	printf("\n");
}

void mcs_tree_barrier(node* n, int* rank, int* numprocs) {
	int data = 1, i;
	MPI_Status status;
	for (i = 0; i < 4; ++i) {
		if (n->have_child[i] != 0) {
			MPI_Recv(&data, 1, MPI_INT, n->have_child[i], 1, MPI_COMM_WORLD, &status);
		}
	}
	if (n->arrival_parent != DUMMY) {
		MPI_Send(&data, 1, MPI_INT, n->arrival_parent, 1, MPI_COMM_WORLD);
	}

	if (*rank != 0) {
		MPI_Recv(&data, 1, MPI_INT, n->wakeup_parent, 1, MPI_COMM_WORLD, &status);
		printf("Process %d is waken up by process %d\n", *rank, n->wakeup_parent);
	}
	if (n->child_ptr[0] != DUMMY) {
		MPI_Send(&data, 1, MPI_INT, n->child_ptr[0], 1, MPI_COMM_WORLD);
		printf("Process %d notifies process %d\n", *rank, n->child_ptr[0]);
	}
	if (n->child_ptr[1] != DUMMY) {
		MPI_Send(&data, 1, MPI_INT, n->child_ptr[1], 1, MPI_COMM_WORLD);
		printf("Process %d notifies process %d\n", *rank, n->child_ptr[1]);
	}
}

int main(int argc, char **argv) {
	node n;
	double time, time1, time2;
	int i, rank, numprocs;
	
	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	init(&n, &rank, &numprocs);
//	print_tree(&n, &rank);

	for (i = 0; i < NUM_BARRIERS; ++i) {
		time1 = MPI_Wtime();
		printf("Process %d reached barrier %d\n", rank, i);
		mcs_tree_barrier(&n, &rank, &numprocs);
		printf("Process %d left barrier %d\n", rank, i);
		time2 = MPI_Wtime();
		time += time2-time1;
	}

	printf("Average time spent in barrier by process %d is %f\n", rank, time/NUM_BARRIERS);

	MPI_Finalize();
	return 0;
}


