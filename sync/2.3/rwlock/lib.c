#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

uint64_t asc_iterations = 0;
uint64_t desc_iterations = 0;
uint64_t eq_iterations = 0;
uint64_t swap_success[3] = {0, 0, 0};

pthread_rwlock_t asc_mtx = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t desc_mtx = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t eq_mtx  = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t swap_mtx[3] = {
    PTHREAD_RWLOCK_INITIALIZER,
    PTHREAD_RWLOCK_INITIALIZER,
    PTHREAD_RWLOCK_INITIALIZER
};

atomic_int stop_flag = 0;

static void random_string(char *buf, size_t len)
{
    static const char letters[] = "abcdefghijklmnopqrstuvwxyz";
    for (size_t i = 0; i < len; ++i)
        buf[i] = letters[rand() % (sizeof(letters) - 1)];
    buf[len] = '\0';
}

Storage *storage_create(size_t n)
{
    Storage *s = malloc(sizeof(Storage));
    if (!s)
        return NULL;
    srand((unsigned)time(NULL));
    s->sentinel = malloc(sizeof(Node));
    if (!s->sentinel) {
        free(s);
        return NULL;
    }
    pthread_rwlock_init(&s->sentinel->sync, NULL);
    s->sentinel->value[0] = '\0';
    s->sentinel->next = NULL;
    s->length = n;

    Node *prev = s->sentinel;
    for (size_t i = 0; i < n; ++i)
    {
        Node *node = malloc(sizeof(Node));
        if (!node) {
            Node *cur = s->sentinel;
            while (cur) {
                Node *next = cur->next;
                pthread_rwlock_destroy(&cur->sync);
                free(cur);
                cur = next;
            }
            free(s);
            return NULL;
        }
        size_t len = 1 + rand() % (MAX_STR - 1);
        random_string(node->value, len);
        node->next = NULL;
        pthread_rwlock_init(&node->sync, NULL);
        prev->next = node;
        prev = node;
    }
    return s;
}

void storage_destroy(Storage *s)
{
    if (!s)
        return;
    Node *cur = s->sentinel;
    while (cur)
    {
        Node *next = cur->next;
        pthread_rwlock_destroy(&cur->sync);
        free(cur);
        cur = next;
    }
    free(s);
}

void *ascending_thread(void *arg)
{
    Storage *s = ((struct ThreadArg *)arg)->s;
    while (!stop_flag)
    {
        Node *prev = s->sentinel;
        pthread_rwlock_rdlock(&prev->sync);
        Node *a = prev->next;
        if (!a)
        {
            pthread_rwlock_unlock(&prev->sync);
            pthread_rwlock_wrlock(&asc_mtx);
            asc_iterations++;
            pthread_rwlock_unlock(&asc_mtx);
            continue;
        }
        pthread_rwlock_rdlock(&a->sync);
        int counter = 1;
        while (1)
        {
            Node *b = a->next;
            if (!b)
            {
                pthread_rwlock_unlock(&a->sync);
                pthread_rwlock_unlock(&prev->sync);
                break;
            }
            pthread_rwlock_rdlock(&b->sync);

            size_t la = strlen(a->value);
            size_t lb = strlen(b->value);
            pthread_rwlock_unlock(&b->sync);
            pthread_rwlock_unlock(&prev->sync);
            if (la < lb)
            {
                counter++;
            }
            prev = a;
            a = a->next;
            pthread_rwlock_rdlock(&a->sync);
        }
        pthread_rwlock_wrlock(&asc_mtx);
        asc_iterations++;
        pthread_rwlock_unlock(&asc_mtx);
    }
    return NULL;
}

void *descending_thread(void *arg)
{
    Storage *s = ((struct ThreadArg *)arg)->s;
    while (!stop_flag)
    {
        Node *prev = s->sentinel;
        pthread_rwlock_rdlock(&prev->sync);
        Node *a = prev->next;
        if (!a)
        {
            pthread_rwlock_unlock(&prev->sync);
            pthread_rwlock_wrlock(&desc_mtx);
            desc_iterations++;
            pthread_rwlock_unlock(&desc_mtx);
            continue;
        }
        pthread_rwlock_rdlock(&a->sync);
        int counter = 0;
        while (1)
        {
            Node *b = a->next;
            if (!b)
            {
                pthread_rwlock_unlock(&a->sync);
                pthread_rwlock_unlock(&prev->sync);
                break;
            }
            pthread_rwlock_rdlock(&b->sync);
            size_t la = strlen(a->value);
            size_t lb = strlen(b->value);
            pthread_rwlock_unlock(&b->sync);
            pthread_rwlock_unlock(&prev->sync);
            if (la > lb)
            {
                counter++;
            }
            prev = a;
            a = a->next;
            pthread_rwlock_rdlock(&a->sync);
        }
        pthread_rwlock_wrlock(&desc_mtx);
        desc_iterations++;
        pthread_rwlock_unlock(&desc_mtx);
    }
    return NULL;
}

void *equal_thread(void *arg)
{
    Storage *s = ((struct ThreadArg *)arg)->s;
    while (!stop_flag)
    {
        Node *prev = s->sentinel;
        pthread_rwlock_rdlock(&prev->sync);
        Node *a = prev->next;
        if (!a)
        {
            pthread_rwlock_unlock(&prev->sync);
            pthread_rwlock_wrlock(&eq_mtx);
            eq_iterations++;
            pthread_rwlock_unlock(&eq_mtx);
            continue;
        }
        pthread_rwlock_rdlock(&a->sync);
        int counter = 0;
        while (1)
        {
            Node *b = a->next;
            if (!b)
            {
                pthread_rwlock_unlock(&a->sync);
                pthread_rwlock_unlock(&prev->sync);
                break;
            }
            pthread_rwlock_rdlock(&b->sync);
            size_t la = strlen(a->value);
            size_t lb = strlen(b->value);
            if (la == lb)
            {
                counter++;
            }
            pthread_rwlock_unlock(&b->sync);
            pthread_rwlock_unlock(&prev->sync);
            prev = a;
            a = a->next;
            pthread_rwlock_rdlock(&a->sync);
        }
        pthread_rwlock_wrlock(&eq_mtx);
        eq_iterations++;
        pthread_rwlock_unlock(&eq_mtx);
    }
    return NULL;
}

void *swap_thread(void *arg)
{
    struct ThreadArg *targ = (struct ThreadArg *)arg;
    Storage *s = targ->s;
    int tid = targ->thread_id;
    while (!stop_flag)
    {
        Node *prev = s->sentinel;
        while (!stop_flag)
        {
            pthread_rwlock_wrlock(&prev->sync);
            Node *a = prev->next;
            if (!a)
            {
                pthread_rwlock_unlock(&prev->sync);
                break;
            }
            pthread_rwlock_wrlock(&a->sync);
            Node *b = a->next;
            if (!b)
            {
                pthread_rwlock_unlock(&a->sync);
                pthread_rwlock_unlock(&prev->sync);
                break;
            }
            if ((rand() & 255) == 0)
            {
                pthread_rwlock_wrlock(&b->sync);
                if (prev->next == a && a->next == b)
                {
                    Node *bnext = b->next;
                    prev->next = b;
                    a->next = bnext;
                    b->next = a;
                }
                pthread_rwlock_unlock(&b->sync);
                pthread_rwlock_unlock(&a->sync);
                pthread_rwlock_unlock(&prev->sync);
                pthread_rwlock_wrlock(&swap_mtx[tid]);
                swap_success[tid]++;
                pthread_rwlock_unlock(&swap_mtx[tid]);
            }
            else
            {
                pthread_rwlock_unlock(&a->sync);
                pthread_rwlock_unlock(&prev->sync);
                prev = prev->next;
                if (!prev)
                    break;
            }
        }

    }
    return NULL;
}

void *monitor_thread(void *arg)
{
    (void)arg;
    while (!stop_flag)
    {
        pthread_rwlock_rdlock(&asc_mtx);
        uint64_t asc = asc_iterations;
        pthread_rwlock_unlock(&asc_mtx);

        pthread_rwlock_rdlock(&desc_mtx);
        uint64_t desc = desc_iterations;
        pthread_rwlock_unlock(&desc_mtx);

        pthread_rwlock_rdlock(&eq_mtx);
        uint64_t eq = eq_iterations;
        pthread_rwlock_unlock(&eq_mtx);

        uint64_t swap[3];
        for (int i = 0; i < 3; i++)
        {
            pthread_rwlock_rdlock(&swap_mtx[i]);
            swap[i] = swap_success[i];
            pthread_rwlock_unlock(&swap_mtx[i]);
        }

        printf("ASC=%lu, DESC=%lu, EQ=%lu, SWAP=[%lu, %lu, %lu]\n",
               asc, desc, eq, swap[0], swap[1], swap[2]);
        sleep(1);
    }
    return NULL;
}