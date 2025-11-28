#ifndef USERTHREAD_H
#define USERTHREAD_H

#include <ucontext.h>

#define STACK_SIZE 2048
#define MAX_THREADS 16

typedef struct {
	void* (*start_routine)(void*);
	void* arg;
	ucontext_t context;
} uthread_struct;

typedef uthread_struct *uthread_t;

int uthread_create(uthread_t* thr, void*(start_routine)(void*), void* arg);
void* uthread_start(void* arg);

typedef struct {
	uthread_t threads[MAX_THREADS];
	unsigned int thread_count;
	unsigned int cur_thread;
} scheduler;

void scheduler_init();
void schedule();
void scheduler_destroy();

#endif 