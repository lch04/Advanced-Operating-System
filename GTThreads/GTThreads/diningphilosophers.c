#include <sys/time.h>
#include <stdio.h>
#include "gtthread.h"

#define CHOPSTICKS 5

#define INITIAL 0
#define EATING 1
#define HUNGRY 2
#define THINKING 3

int philosopher_status[CHOPSTICKS];

gtthread_mutex_t chopstick_mutex[CHOPSTICKS];

void* dine(int* index) {
	int id, eating, thinking, i, j;
	printf("I'm philosopher %d\n", *index);
	id = *index;

	for (;;) {
		philosopher_status[id] = THINKING;
		printf("Philosopher %d is thinking\n", id);
		thinking = rand()%10000000;
		for(i = 0; i < thinking; i++);

		philosopher_status[id]=HUNGRY;
		printf("Philosopher %d is hungry\n", id);

		while(philosopher_status[id+CHOPSTICKS-1] == EATING || philosopher_status[id+1] == EATING);

		gtthread_mutex_lock(&chopstick_mutex[id]);
		gtthread_mutex_lock(&chopstick_mutex[(id+1)%CHOPSTICKS]);

		philosopher_status[id]=EATING;
		printf("Philosopher %d is eating\n", id);
		eating = rand()%10000000;
		for(j = 0; j < eating; j++);
		printf("Philosopher %d is done eating\n", id);

		gtthread_mutex_unlock(&chopstick_mutex[(id+1)%CHOPSTICKS]);
		gtthread_mutex_unlock(&chopstick_mutex[id]);
	}
	gtthread_exit(NULL);
}

int main(void) {
	int i;
	gtthread_t philosopher[CHOPSTICKS];
	int id[CHOPSTICKS];
	gtthread_init(10000);

	for (i = 0; i < CHOPSTICKS; i++) {
		gtthread_mutex_init(&chopstick_mutex[i]);
		philosopher_status[i] = INITIAL;
	}

	for (i = 0; i < CHOPSTICKS; i++) {
		id[i] = i;
		gtthread_create(&philosopher[i], (void*(*)(void*))dine, &id[i]);
	}

	gtthread_exit(0);
	return 0;
}

