#ifndef __MYSYNC__
#define __MYSYNC__

#include <stdatomic.h>
#include <sys/types.h>

#define STATUS_LOCK   0
#define STATUS_UNLOCK 1
#define NO_TID       -1

typedef enum {
    MUTEX_NORMAL,
    MUTEX_RECURSIVE,
    MUTEX_TRY
} mutex_type_t;

typedef struct {
    _Atomic int lock;
    _Atomic pid_t tid;
    _Atomic int recursion;
    mutex_type_t type;
} mutex_t;

/* spinlock остаётся как есть */
typedef struct {
    _Atomic int state;
} spinlock_t;

void spin_init(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
int  spin_unlock(spinlock_t *lock);

/* mutex API */
void mutex_init(mutex_t *m, mutex_type_t type);
void mutex_lock(mutex_t *m);
int  mutex_trylock(mutex_t *m);
int  mutex_unlock(mutex_t *m);

#endif