#include "uthread.h"
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>

scheduler sched;
ucontext_t main_context;

int uthread_create(uthread_t *thr, void *(*start_routine)(void *), void *arg)
{
	void *region = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
						MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (region == MAP_FAILED)
		return -1;

	uthread_t thread = malloc(sizeof(uthread_struct));
	if (!thread)
	{
		munmap(region, STACK_SIZE);
		return -1;
	}

	if (getcontext(&thread->context) == -1)
	{
		free(thread);
		munmap(region, STACK_SIZE);
		return -1;
	}

	thread->context.uc_stack.ss_sp = region;
	thread->context.uc_stack.ss_size = STACK_SIZE;
	thread->context.uc_link = &main_context;

	thread->start_routine = start_routine;
	thread->arg = arg;
	thread->state = STATE_READY;
	thread->wakeup_time = 0;

	makecontext(&thread->context, (void (*)(void))uthread_start, 1, thread);

	// Добавляем в двусвязный список
	thread->prev = sched.tail;
	thread->next = NULL;
	if (sched.tail)
		sched.tail->next = thread;
	sched.tail = thread;
	if (!sched.head)
		sched.head = thread;

	sched.thread_count++;
	*thr = thread;
	return 0;
}

void *uthread_start(void *arg)
{
	uthread_t thr = (uthread_t)arg;
	thr->start_routine(thr->arg);

	thr->state = STATE_TERMINATED;
	// sched.cur_thread = thr->next ? thr->next : sched.head;

	if (thr->prev)
		thr->prev->next = thr->next;
	if (thr->next)
		thr->next->prev = thr->prev;
	if (sched.head == thr)
		sched.head = thr->next;
	if (sched.tail == thr)
		sched.tail = thr->prev;

	sched.cleanup[sched.cleanup_count++] = thr;
	sched.thread_count--;

	schedule(); 
	return NULL;
}

void scheduler_init()
{
	sched.head = sched.tail = NULL;
	sched.cur_thread = NULL;
	sched.thread_count = 0;
	sched.cleanup_count = 0;

	uthread_t main_thread = malloc(sizeof(uthread_struct));
	if (getcontext(&main_thread->context) == -1)
	{
		printf("ERROR: getcontext() failed\n");
		exit(-1);
	}
	main_context = main_thread->context;
	main_thread->state = STATE_READY;
	main_thread->prev = main_thread->next = NULL;

	sched.head = sched.tail = sched.cur_thread = main_thread;
	sched.thread_count = 1;
}

uint64_t now_ms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void schedule()
{
	if (!sched.head)
		return; // нет потоков для запуска

	while (1)
	{
		unsigned long time = now_ms();
		uthread_t thr = sched.cur_thread ? sched.cur_thread : sched.head;
		uthread_t start = thr;

		int found_ready = 0;
		int has_sleeping = 0;

		do
		{
			thr = thr->next ? thr->next : sched.head;
			if (thr->state == STATE_SLEEPING)
			{
				has_sleeping = 1;
				if (thr->wakeup_time <= now_ms())
					thr->state = STATE_READY;
			}

			if (thr->state != STATE_SLEEPING && thr->state != STATE_TERMINATED)
			{
				found_ready = 1;
				break;
			}


		} while (thr != start);

		if (!found_ready)
		{
			if (has_sleeping)
				continue;
			else
				break;
		}

		uthread_t prev = sched.cur_thread ? sched.cur_thread : sched.head;
		sched.cur_thread = thr;
		thr->state = STATE_RUNNING;

		if (prev->state != STATE_SLEEPING && prev->state != STATE_TERMINATED)
			prev->state = STATE_READY;

		if (prev != thr)
		{
			swapcontext(&prev->context, &thr->context);
		}
		else
		{
			break; // продолжаем текущий поток, других нет
		}	
		break; // после swapcontext вернемся сюда уже другой поток, так что break нужен
	}
}

void scheduler_destroy()
{
	for (unsigned int i = 0; i < sched.cleanup_count; i++)
	{
		munmap(sched.cleanup[i]->context.uc_stack.ss_sp, STACK_SIZE);
		free(sched.cleanup[i]);
	}

	// основной поток
	if (sched.head)
		free(sched.head);
}

void uthread_sleep(unsigned long ms)
{
	uthread_t thr = sched.cur_thread;
	thr->state = STATE_SLEEPING;
	thr->wakeup_time = now_ms() + ms;

	schedule();
}