#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>

uint64_t asc_iterations = 0;
uint64_t desc_iterations = 0;
uint64_t eq_iterations = 0;
uint64_t swap_success[3] = {0, 0, 0};

pthread_mutex_t asc_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t desc_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t eq_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t swap_mtx[3] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};



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
    if (pthread_spin_init(&s->sentinel->sync, 0) != 0) {
        free(s->sentinel);
        free(s);
        return NULL;
    }
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
                pthread_spin_destroy(&cur->sync);
                free(cur);
                cur = next;
            }
            free(s);
            return NULL;
        }
        size_t len = 1 + rand() % (MAX_STR - 1);
        random_string(node->value, len);
        node->next = NULL;
        if (pthread_spin_init(&node->sync, 0) != 0) {
            free(node);
            Node *cur = s->sentinel;
            while (cur) {
                Node *next = cur->next;
                pthread_spin_destroy(&cur->sync);
                free(cur);
                cur = next;
            }
            free(s);
            return NULL;
        }
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
        pthread_spin_destroy(&cur->sync);
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
        pthread_spin_lock(&prev->sync);
        Node *a = prev->next;
        if (!a)
        {
            pthread_spin_unlock(&prev->sync);
            pthread_mutex_lock(&asc_mtx);
            asc_iterations++;
            pthread_mutex_unlock(&asc_mtx);
            continue;
        }
        pthread_spin_lock(&a->sync);
        int counter = 0;
        while (1)
        {
            Node *b = a->next;
            if (!b)
            {
                pthread_spin_unlock(&a->sync);
                pthread_spin_unlock(&prev->sync);
                break;
            }
            pthread_spin_lock(&b->sync);

            size_t la = strlen(a->value);
            size_t lb = strlen(b->value);

            pthread_spin_unlock(&b->sync);
            pthread_spin_unlock(&prev->sync);
            if (la < lb)
            {
                counter++;
            }

            prev = a;
            a = a->next;
            pthread_spin_lock(&a->sync);
        }
        pthread_mutex_lock(&asc_mtx);
        asc_iterations++;
        pthread_mutex_unlock(&asc_mtx);
    }
    return NULL;
}

void *descending_thread(void *arg)
{
    Storage *s = ((struct ThreadArg *)arg)->s;
    while (!stop_flag)
    {
        Node *prev = s->sentinel;
        pthread_spin_lock(&prev->sync);
        Node *a = prev->next;
        if (!a)
        {
            pthread_spin_unlock(&prev->sync);
            pthread_mutex_lock(&desc_mtx);
            desc_iterations++;
            pthread_mutex_unlock(&desc_mtx);
            continue;
        }
        pthread_spin_lock(&a->sync);
        int counter = 0;
        while (1)
        {
            Node *b = a->next;
            if (!b)
            {
                pthread_spin_unlock(&a->sync);
                pthread_spin_unlock(&prev->sync);
                break;
            }
            pthread_spin_lock(&b->sync);
            size_t la = strlen(a->value);
            size_t lb = strlen(b->value);
            pthread_spin_unlock(&b->sync);
            pthread_spin_unlock(&prev->sync);
            if (la > lb)
            {
                counter++;
            }
            prev = a;
            a = a->next;
            pthread_spin_lock(&a->sync);
        }
        pthread_mutex_lock(&desc_mtx);
        desc_iterations++;
        pthread_mutex_unlock(&desc_mtx);
    }
    return NULL;
}

void *equal_thread(void *arg)
{
    Storage *s = ((struct ThreadArg *)arg)->s;
    while (!stop_flag)
    {
        Node *prev = s->sentinel;
        pthread_spin_lock(&prev->sync);
        Node *a = prev->next;
        if (!a)
        {
            pthread_spin_unlock(&prev->sync);
            pthread_mutex_lock(&eq_mtx);
            eq_iterations++;
            pthread_mutex_unlock(&eq_mtx);
            continue;
        }
        pthread_spin_lock(&a->sync);
        int counter = 0;
        while (1)
        {
            Node *b = a->next;
            if (!b)
            {
                pthread_spin_unlock(&a->sync);
                pthread_spin_unlock(&prev->sync);
                break;
            }
            pthread_spin_lock(&b->sync);
            size_t la = strlen(a->value);
            size_t lb = strlen(b->value);
            pthread_spin_unlock(&b->sync);
            pthread_spin_unlock(&prev->sync);
            if (la == lb)
            {
                counter++;
            }

            prev = a;
            a = a->next;
            pthread_spin_lock(&a->sync);
        }
        pthread_mutex_lock(&eq_mtx);
        eq_iterations++;
        pthread_mutex_unlock(&eq_mtx);
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
            pthread_spin_lock(&prev->sync);
            Node *a = prev->next;
            if (!a)
            {
                pthread_spin_unlock(&prev->sync);
                break;
            }
            pthread_spin_lock(&a->sync);
            Node *b = a->next;
            if (!b)
            {
                pthread_spin_unlock(&a->sync);
                pthread_spin_unlock(&prev->sync);
                break;
            }
            if ((rand() & 255) == 0)
            {
                pthread_spin_lock(&b->sync);
                if (prev->next == a && a->next == b)
                {
                    Node *bnext = b->next;
                    prev->next = b;
                    a->next = bnext;
                    b->next = a;
                }
                pthread_spin_unlock(&b->sync);
                pthread_spin_unlock(&a->sync);
                pthread_spin_unlock(&prev->sync);
            }
            else
            {
                pthread_spin_unlock(&a->sync);
                pthread_spin_unlock(&prev->sync);
                prev = prev->next;
                if (!prev)
                    break;
            }
        }
        pthread_mutex_lock(&swap_mtx[tid]);
        swap_success[tid]++;
        pthread_mutex_unlock(&swap_mtx[tid]);
    }
    return NULL;
}

void *monitor_thread(void *arg)
{
    (void)arg;
    while (!stop_flag)
    {
        pthread_mutex_lock(&asc_mtx);
        uint64_t asc = asc_iterations;
        pthread_mutex_unlock(&asc_mtx);

        pthread_mutex_lock(&desc_mtx);
        uint64_t desc = desc_iterations;
        pthread_mutex_unlock(&desc_mtx);

        pthread_mutex_lock(&eq_mtx);
        uint64_t eq = eq_iterations;
        pthread_mutex_unlock(&eq_mtx);

        uint64_t swap[3];
        for (int i = 0; i < 3; i++)
        {
            pthread_mutex_lock(&swap_mtx[i]);
            swap[i] = swap_success[i];
            pthread_mutex_unlock(&swap_mtx[i]);
        }

        printf("ASC=%lu, DESC=%lu, EQ=%lu, SWAP=[%lu, %lu, %lu]\n",
               asc, desc, eq, swap[0], swap[1], swap[2]);
        sleep(1);
    }
    return NULL;
}