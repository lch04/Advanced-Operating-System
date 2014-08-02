#include "gtthread.h"

#define MAX_NUM_THREADS 24
#define MAIN_THREAD 0
#define SCHEDULED_THREAD 1
#define THREAD_STACK_SIZE 1024*64

gtthread *current_thread, idle_thread;
struct itimerval timer;
struct sigaction action;
gtthread gtthreads[MAX_NUM_THREADS];
sigset_t signal_mask;
ucontext_t *scheduled_context;

void idle() {
	sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
	for(;;);
}

void enqueue(gtthread* entry_thread, gtthread* new_thread)
{
    new_thread->next = entry_thread->next;
    new_thread->prev = entry_thread;
    if (entry_thread->next != NULL) {
		entry_thread->next->prev = new_thread;
    }
    entry_thread->next = new_thread;
}

void dequeue(gtthread* thread)
{
    gtthread* prev_ptr = thread->prev;
    gtthread* next_ptr = thread->next;
    if (prev_ptr == thread && next_ptr == thread) {
        current_thread = NULL;
        return;
    }
    prev_ptr->next = next_ptr;
    next_ptr->prev = prev_ptr;
}

void recycle(gtthread* thread) {
	if (thread != NULL) {
		free(thread->context.uc_stack.ss_sp);
	}
	if (thread->join != NULL) {
		gtthread* prev_thread = thread->prev;
		gtthread* join_thread = thread->join;
		enqueue(prev_thread, join_thread);
		thread->join = NULL;
	}
	dequeue(thread);
}

void preemptive_scheduler() {
	DEBUG_OUT("Start to execute scheduler");

    sigprocmask(SIG_BLOCK, &signal_mask, NULL);

    DEBUG_OUT1("[preemptive_scheduler] current thread is %d", (int)current_thread->id);

    if (current_thread->running) {
    	current_thread = current_thread->next;
    }

    DEBUG_OUT1("[preemptive_scheduler] current thread becomes %d", (int)current_thread->id);

    while (!current_thread->running) {
    	DEBUG_OUT1("[preemptive_scheduler] current thread %d is not active!", (int)current_thread->id);
        recycle(current_thread);
        if (current_thread == NULL) {
        	DEBUG_OUT("[preemptive_scheduler] EXITING!");
            exit(0);
        }
        current_thread = current_thread->next;
    }

    DEBUG_OUT1("[preemptive_scheduler] current thread becomes %d", (int)current_thread->id);

    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    setcontext(&current_thread->context);
}

void signal_handler(int signum, siginfo_t* siginfo, void* ctx)
{
    static int counter = 0;
    ++counter;

    DEBUG_OUT1("[signal_handler] signal handler is called %d number of times", counter);

	if (!current_thread->running) {
        setcontext(scheduled_context);
    } else if (current_thread->yield) {
        current_thread->yield = 0;
        setcontext(scheduled_context);
    } else {
        swapcontext(&current_thread->context, scheduled_context);
    }
}

void gtthread_init(long period) {
	int i;

    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGPROF);

    DEBUG_OUT("Init signal mask successfully.");

    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = period;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = period;

    DEBUG_OUT("Init timer successfully.");

    memset(&action, '\0', sizeof(action));
    action.sa_sigaction = &signal_handler;
    action.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&action.sa_mask);
    sigaction(SIGPROF, &action, NULL);

    DEBUG_OUT("Init signal action successfully.");

	for (i = 0; i < MAX_NUM_THREADS; ++i) {
		gtthreads[i].id = (gtthread_t)i;
		gtthreads[i].prev = NULL;
		gtthreads[i].next = NULL;
		gtthreads[i].return_value = NULL;
		gtthreads[i].join = NULL;
		gtthreads[i].running = 0;
		gtthreads[i].used = 0;
		gtthreads[i].yield = 0;
	}

	DEBUG_OUT("Init threads successfully.");

	gtthreads[MAIN_THREAD].running = 1;
	gtthreads[MAIN_THREAD].used = 1;
	gtthreads[MAIN_THREAD].next = &gtthreads[MAIN_THREAD];
	gtthreads[MAIN_THREAD].prev = &gtthreads[MAIN_THREAD];
	getcontext(&gtthreads[MAIN_THREAD].context);
	gtthreads[MAIN_THREAD].context.uc_stack.ss_flags = 0;
	gtthreads[MAIN_THREAD].context.uc_stack.ss_size = THREAD_STACK_SIZE;
	gtthreads[MAIN_THREAD].context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
	gtthreads[MAIN_THREAD].context.uc_link = NULL;

	DEBUG_OUT("Init main thread successfully.");

	getcontext(&idle_thread.context);
	idle_thread.context.uc_stack.ss_flags = 0;
	idle_thread.context.uc_stack.ss_size = THREAD_STACK_SIZE;
	idle_thread.context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
	idle_thread.context.uc_link = NULL;
	makecontext(&idle_thread.context, idle, 0);

	DEBUG_OUT("Init idle thread successfully.");

    scheduled_context = &gtthreads[SCHEDULED_THREAD].context;
    gtthreads[SCHEDULED_THREAD].used = 1;
    getcontext(scheduled_context);
    scheduled_context->uc_stack.ss_flags = 0;
    scheduled_context->uc_stack.ss_size = THREAD_STACK_SIZE;
    scheduled_context->uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
    scheduled_context->uc_link = NULL;
    makecontext(scheduled_context, preemptive_scheduler, 0);

    DEBUG_OUT("Init schedule thread successfully.");

    current_thread = &gtthreads[MAIN_THREAD];
    setitimer(ITIMER_PROF, &timer, NULL);
    swapcontext(&current_thread->context, scheduled_context);

    DEBUG_OUT("Calling scheduled_context...");

    fflush(stdout);
}

void gtthread_execute(gtthread* thread, void *(*start_routine)(void *), void *arg)
{
    thread->return_value = start_routine(arg);
    thread->running = 0;
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    for(;;);
}

int gtthread_create(gtthread_t *thread,
                    void *(*start_routine)(void *),
                    void *arg)
{
    int i, id = -1;
    sigprocmask(SIG_BLOCK, &signal_mask, NULL);
    for (i = 2; i < MAX_NUM_THREADS; ++i) {
        if (!gtthreads[i].used) {
            gtthreads[i].used = 1;
            id = i;
            break;
        }
    }

    if (id == -1) {
    	return 1;
    }

    DEBUG_OUT1("[gtthread_create] found a free spot %d", id);

    *thread = (gtthread_t)id;
    gtthreads[id].id = (gtthread_t)(id);
    gtthreads[id].running = 1;
    getcontext(&gtthreads[id].context);
    gtthreads[id].context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
    gtthreads[id].context.uc_stack.ss_size = THREAD_STACK_SIZE;
    gtthreads[id].context.uc_stack.ss_flags = 0;
    gtthreads[id].context.uc_link = NULL;
    sigemptyset(&gtthreads[id].context.uc_sigmask);
    makecontext(&gtthreads[id].context, (void(*)(void))gtthread_execute, 3, &gtthreads[id], start_routine, arg);

    enqueue(current_thread, &gtthreads[id]);
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);

    return 0;
}

int gtthread_join(gtthread_t thread, void **status)
{
    gtthread_t self = gtthread_self();
    gtthread* joining_thread = &gtthreads[(int)thread];

    if (thread < 0 || (int)thread >= MAX_NUM_THREADS)
        return 1;

    if (gtthread_equal(thread, self))
        return 1;

    if (!joining_thread->used || joining_thread->join != NULL)
        return 1;

    if (!joining_thread->running) {
        sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
        if (status != NULL)
            *status = joining_thread->return_value;
        return 0;
    }

    sigprocmask(SIG_BLOCK, &signal_mask, NULL);
    dequeue(current_thread);
    joining_thread->join = current_thread;
    gtthread_yield();

    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    if (status != NULL) {
        *status = joining_thread->return_value;
    }
    return 0;
}

int gtthread_cancel(gtthread_t thread)
{
	gtthread* temp = &gtthreads[(int)thread];

	if (thread < 0 || (int)thread >= MAX_NUM_THREADS)
        return 1;

    if (!temp->used)
        return 1;

    temp->running = 0;
    return 0;
}

void gtthread_exit(void *retval) {
	current_thread->return_value = retval;
	current_thread->running = 0;
	gtthread_yield();
}

void gtthread_yield(void) {
    sigprocmask(SIG_BLOCK, &signal_mask, NULL);
    current_thread->yield = 1;
    swapcontext(&current_thread->context, &idle_thread.context);
}

gtthread_t gtthread_self(void) {
	return current_thread->id;
}

int gtthread_equal(gtthread_t t1, gtthread_t t2)
{
    return (t1 == t2) ? 1:0;
}

int gtthread_mutex_init(gtthread_mutex_t *mutex) {
	if (mutex->lock == 1) {
		return -1;
	}
	mutex->lock = 0;
	mutex->owner = -1;
	return 0;
}

int gtthread_mutex_lock(gtthread_mutex_t *mutex) {
	if (mutex->owner == current_thread->id) {
		return -1;
	}
	while (mutex->lock != 0) {
		gtthread_yield();
	}
	mutex->lock = 1;
	mutex->owner = current_thread->id;
	return 0;
}

int gtthread_mutex_unlock(gtthread_mutex_t *mutex) {
	if (mutex->lock == 1 && mutex->owner == current_thread->id) {
		mutex->lock = 0;
		mutex->owner = -1;
		return 0;
	} else {
		return -1;
	}
}

