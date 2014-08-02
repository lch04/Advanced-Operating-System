#include <stdio.h>
#include "mpi.h"
#include <stdlib.h>

#define NUM_BARRIERS 1024

void centralized_barrier(int* rank, int* numprocs, int* sense) {
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
    int i;
    int rank, numprocs, sense = 0;
    double time, time1, time2;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (i = 0; i < NUM_BARRIERS; ++i) {
        printf("Process %d entered barrier %d\n", rank, i);
        time1 = MPI_Wtime();
        centralized_barrier(&rank, &numprocs, &sense);
        time2 = MPI_Wtime();
        time += (time2 - time1);
        printf("Process %d left barrier %d\n", rank, i);
    }

    printf("Average time spent in barrier by process %d is %f\n", rank, time/NUM_BARRIERS);

    MPI_Finalize();
    return 0;
}


