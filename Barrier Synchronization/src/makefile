all: centralized mcs combined omp-centralized omp-mcs

centralized: centralized.c 
	/opt/openmpi-1.4.3-gcc44/bin/mpicc -o centralized centralized.c -lm

mcs: mcs_tree.c mcs_tree.h
	/opt/openmpi-1.4.3-gcc44/bin/mpicc -o mcs mcs_tree.c -lm

combined: combined.c mcs_tree.h
	/opt/openmpi-1.4.3-gcc44/bin/mpicc -fopenmp -o combined combined.c -lm
	
omp-centralized: omp-centralized.c
	gcc -fopenmp omp-centralized.c -o omp-centralized

omp-centralized: omp-mcs.c
	gcc -fopenmp omp-mcs.c -o omp-mcs
	
clean:
	rm -rf *.o centralized mcs combined omp-centralized omp-mcs


