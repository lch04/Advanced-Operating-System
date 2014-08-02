#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <malloc.h>
#include <signal.h>
#include <ucontext.h>
#include <string.h>

typedef unsigned long gtthread_t;

typedef struct gtthread {
	gtthread_t id;
	struct gtthread* join;
	ucontext_t context;			/* */
	struct gtthread* prev;
	struct gtthread* next;
	void* return_value;
	int running;
	int yield;
	int used;
} gtthread;

typedef struct gtthread_mutex {
	long lock;
	gtthread_t owner;
} gtthread_mutex_t;

void gtthread_init(long period);
int  gtthread_create(gtthread_t *thread, void *(*start_routine)(void *), void *arg);
int  gtthread_join(gtthread_t thread, void **status);
int  gtthread_cancel(gtthread_t thread);

void gtthread_exit(void *retval);
void gtthread_yield(void);
int  gtthread_equal(gtthread_t t1, gtthread_t t2);
gtthread_t gtthread_self(void);
int  gtthread_mutex_init(gtthread_mutex_t *mutex);
int  gtthread_mutex_lock(gtthread_mutex_t *mutex);
int  gtthread_mutex_unlock(gtthread_mutex_t *mutex);

#ifdef DEBUG

#define	DEBUG_OUT(string)    printf( "[DEBUG]: " string "\n"); fflush(stdout);
#define	DEBUG_OUT1(string, arg)   printf( "[DEBUG]: " string ": " arg "\n")

#else

#define DEBUG_OUT(string)
#define DEBUG_OUT1(string, arg)

#endif

