#define _GNU_SOURCE

#include "lib.h"
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>


pid_t gettid(void) {
    return syscall(SYS_gettid);
}

#define THREADS 6
#define ITER    1000000

int shared_counter;

typedef struct {
    mutex_t *mutex;
    mutex_type_t type;
} thread_arg_t;

void *thread_func(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    mutex_t *m = targ->mutex;

    for (int i = 0; i < ITER; i++) {
        if (targ->type == MUTEX_TRY) {
            // пробуем заблокировать, если не удалось — просто пропускаем
            mutex_trylock(m);
        } else {
            mutex_lock(m);
        }

        shared_counter++;

        if (targ->type != MUTEX_TRY || atomic_load(&m->tid) == gettid()) {
            mutex_unlock(m);
        }
    }

    return NULL;
}

void test_mutex(mutex_type_t type, const char *name) {
    pthread_t threads[THREADS];
    mutex_t m;

    mutex_init(&m, type);
    shared_counter = 0;

    thread_arg_t targ = { .mutex = &m, .type = type };

    for (int i = 0; i < THREADS; i++)
        pthread_create(&threads[i], NULL, thread_func, &targ);

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("%s result: %d (expected %d or less for TRY)\n",
           name, shared_counter, THREADS * ITER);
}

int main(void) {
    test_mutex(MUTEX_NORMAL, "NORMAL");
    test_mutex(MUTEX_RECURSIVE, "RECURSIVE");
    test_mutex(MUTEX_TRY, "TRY");

    return 0;
}