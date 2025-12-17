#include <stdatomic.h>
#define _GNU_SOURCE

#include <errno.h>
#include <linux/futex.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "lib.h"

static int futex(int *uaddr, int op, int val)
{
    return syscall(SYS_futex, uaddr, op, val, NULL, NULL, 0);
}

/* ---------------- SPINLOCK ---------------- */

void spin_init(spinlock_t *lock)
{
    atomic_store(&lock->state, STATUS_UNLOCK);
}

void spin_lock(spinlock_t *lock)
{
    int expected;
    while (1) {
        expected = STATUS_UNLOCK;
        if (atomic_compare_exchange_strong(&lock->state, &expected, STATUS_LOCK))
            return;
    }
}

int spin_unlock(spinlock_t *lock)
{
    int expected = STATUS_LOCK;
    return atomic_compare_exchange_strong(&lock->state, &expected, STATUS_UNLOCK)
           ? 0 : -1;
}

/* ---------------- MUTEX ---------------- */

void mutex_init(mutex_t *m, mutex_type_t type)
{
    atomic_store(&m->lock, STATUS_UNLOCK);
    atomic_store(&m->tid, NO_TID);
    atomic_store(&m->recursion, 0);
    m->type = type;
}

void mutex_lock(mutex_t *m)
{
    pid_t self = gettid();

    /* RECURSIVE */
    if (m->type == MUTEX_RECURSIVE &&
        atomic_load(&m->tid) == self)
    {
        atomic_fetch_add(&m->recursion, 1);
        return;
    }

    while (1) {
        int expected = STATUS_UNLOCK;
        if (atomic_compare_exchange_strong(&m->lock, &expected, STATUS_LOCK)) {
            atomic_store(&m->tid, self);
            atomic_store(&m->recursion, 1);
            return;
        }
        futex((int *)&m->lock, FUTEX_WAIT, STATUS_LOCK);
    }
}

int mutex_trylock(mutex_t *m)
{
    pid_t self = gettid();

    if (m->type == MUTEX_RECURSIVE &&
        atomic_load(&m->tid) == self)
    {
        atomic_fetch_add(&m->recursion, 1);
        return 0;
    }

    int expected = STATUS_UNLOCK;
    if (!atomic_compare_exchange_strong(&m->lock, &expected, STATUS_LOCK))
        return -1;

    atomic_store(&m->tid, self);
    atomic_store(&m->recursion, 1);
    return 0;
}

int mutex_unlock(mutex_t *m)
{
    if (atomic_load(&m->tid) != gettid())
        return -1;

    /* RECURSIVE */
    if (m->type == MUTEX_RECURSIVE) {
        int r = atomic_fetch_sub(&m->recursion, 1);
        if (r > 1)
            return 0;
    }

    atomic_store(&m->tid, NO_TID);
    atomic_store(&m->lock, STATUS_UNLOCK);
    futex((int *)&m->lock, FUTEX_WAKE, 1);
    return 0;
}