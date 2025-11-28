#include "uthread.h"
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

scheduler sched;

int uthread_create(uthread_t* thr, void*(start_routine)(void*), void* arg) {
	void* region = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (region == MAP_FAILED) {
		printf("ERROR in ithread_create: mmap() failed\n");
		return -1;
	}

	uthread_t thread = (uthread_t)((char*)region + STACK_SIZE - sizeof(uthread_struct));

	int err = getcontext(&thread->context);
	if (err == -1) {
		printf("ERROR in ithread_create: getcontext() failed\n");
		return -1;
	}

	thread->context.uc_stack.ss_sp = region;
	thread->context.uc_stack.ss_size = STACK_SIZE - sizeof(uthread_t);
	thread->context.uc_link = NULL;
	makecontext(&thread->context, (void (*)(void))uthread_start, 1, thread);

	thread->start_routine = start_routine;
	thread->arg = arg;

	if (sched.thread_count >= MAX_THREADS - 1) {
		printf("ERROR in ithread_create: too many threads\n");
		return -1;
	}
	sched.threads[sched.thread_count++] = thread;
	*thr = thread;
	return 0;
}

void* uthread_start(void* arg) {
	uthread_t thr = (uthread_t)arg;
	thr->start_routine(thr->arg);
	return NULL;
}

void scheduler_init() {
	sched.thread_count = 0;
	sched.cur_thread = 0;

	uthread_t main_thread = malloc(sizeof(uthread_struct));
	if (getcontext(&(main_thread->context)) == -1) {
		printf("ERROR in scheduler_init: getcontext() failed\n");
		exit(-1);
	}
	sched.threads[sched.thread_count++] = main_thread;
}

void schedule() {
	ucontext_t* current_context = &(sched.threads[sched.cur_thread]->context);
	sched.cur_thread = (sched.cur_thread + 1) % sched.thread_count;
	ucontext_t* next_context = &(sched.threads[sched.cur_thread]->context);

	int err = swapcontext(current_context, next_context);
	if (err == -1) {
		printf("ERROR in scheduler_init: swapcontext() failed\n");
		exit(-1);
	}
}

void scheduler_destroy() {
	free(sched.threads[0]);
	for(int i = 1; i < sched.thread_count; ++i) {
		munmap(sched.threads[i]->context.uc_stack.ss_sp, STACK_SIZE);
	}
}