#ifndef USERTHREAD_H
#define USERTHREAD_H

#include <ucontext.h>
#include <stdint.h>

#define STACK_SIZE 2048
#define MAX_THREADS 16

#define STATE_READY     0
#define STATE_RUNNING   1
#define STATE_SLEEPING  2
#define STATE_TERMINATED 3

typedef struct uthread_struct {
    void* (*start_routine)(void*);
    void* arg;
    ucontext_t context;
    int state;
    uint64_t wakeup_time; // ms

    struct uthread_struct* prev;
    struct uthread_struct* next;
} uthread_struct;

typedef uthread_struct* uthread_t;

int uthread_create(uthread_t* thr, void*(start_routine)(void*), void* arg);
void* uthread_start(void* arg);

typedef struct {
    uthread_t head;  
    uthread_t tail;
    unsigned int thread_count;

    uthread_t cleanup[MAX_THREADS];
    unsigned int cleanup_count;

    uthread_t cur_thread;
} scheduler;

void uthread_sleep(unsigned long ms);
void scheduler_init();
void schedule();
void scheduler_destroy();


#endif 