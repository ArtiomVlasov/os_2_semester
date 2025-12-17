#define _GNU_SOURCE
#include "cache.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

cache_t global_cache;

unsigned long hash_str(const char *s)
{
    unsigned long h = 5381;
    while (*s)
        h = ((h << 5) + h) + (unsigned char)(*s++);
    return h % CACHE_HASH_SIZE;
}

void cache_init(cache_t *c)
{
    memset(c, 0, sizeof(*c));
    pthread_mutex_init(&c->lock, NULL);
    c->lru_head = c->lru_tail = NULL;
    c->total_size = 0;
}

void lru_remove(cache_t *c, cache_entry_t *e)
{
    if (!e)
        return;
    if (e->prev)
        e->prev->next = e->next;
    else
        c->lru_head = e->next;
    if (e->next)
        e->next->prev = e->prev;
    else
        c->lru_tail = e->prev;
    e->prev = e->next = NULL;
}

void lru_insert_head(cache_t *c, cache_entry_t *e)
{
    e->prev = NULL;
    e->next = c->lru_head;
    if (c->lru_head)
        c->lru_head->prev = e;
    c->lru_head = e;
    if (!c->lru_tail)
        c->lru_tail = e;
}

void promote_lru(cache_t *c, cache_entry_t *e)
{
    lru_remove(c, e);
    lru_insert_head(c, e);
}

cache_entry_t *cache_find(cache_t *c, const char *key)
{
    unsigned long h = hash_str(key);
    cache_entry_t *cur = c->buckets[h];
    while (cur)
    {
        if (strcmp(cur->key, key) == 0)
            return cur;
        cur = cur->hnext;
    }
    return NULL;
}

void cache_insert(cache_t *c, cache_entry_t *e)
{
    unsigned long h = hash_str(e->key);
    e->hnext = c->buckets[h];
    c->buckets[h] = e;
    lru_insert_head(c, e);
    c->total_size += e->size;
}

void cache_remove_entry(cache_t *c, cache_entry_t *e)
{
    if (!c || !e)
        return;
    unsigned long h = hash_str(e->key);
    cache_entry_t **p = &c->buckets[h];
    while (*p)
    {
        if (*p == e)
        {
            *p = e->hnext;
            break;
        }
        p = &(*p)->hnext;
    }
    lru_remove(c, e);
    c->total_size -= e->size;
    if (e->key)
        free(e->key);
    if (e->data)
        free(e->data);
    pthread_mutex_destroy(&e->lock);
    pthread_cond_destroy(&e->cond);
    free(e);
}


void cache_evict_if_needed(cache_t *c)
{

    while (MAX_CACHE_TOTAL > 0 && c->total_size > MAX_CACHE_TOTAL)
    {
        cache_entry_t *tail = c->lru_tail;
        if (!tail)
            break;
        log_msg("Evicting key=%s size=%zu (LRU)", tail->key, tail->size);
        cache_remove_entry(c, tail);
    }
}

void *cache_maintenance_thread(void *arg)
{
    cache_t *c = (cache_t *)arg;
    (void)arg;
    for (;;)
    {
        sleep(5);
        time_t now = time(NULL);
        pthread_mutex_lock(&c->lock);
        int evicted = 0;
        cache_entry_t *cur = c->lru_tail;
        while (cur)
        {
            cache_entry_t *prev = cur->prev;
            if (cur->expire_at != 0 && cur->expire_at <= now)
            {
                log_msg("TTL expired, evict key=%s", cur->key);
                cache_remove_entry(c, cur);
                evicted++;
                if (evicted >= EVICT_BATCH)
                    break;
            }
            cur = prev;
        }
        cache_evict_if_needed(c);
        pthread_mutex_unlock(&c->lock);
    }
    return NULL;
}

cache_entry_t *cache_entry_create(const char *key, time_t ttl)
{
    cache_entry_t *e = calloc(1, sizeof(*e));
    if (!e)
        return NULL;
    e->key = strdup(key);
    e->data = NULL;
    e->size = 0;
    e->capacity = 0;
    e->loading = 1;
    e->complete = 0;
    e->last_access = time(NULL);
    e->expire_at = (ttl > 0) ? (time(NULL) + ttl) : 0;
    pthread_mutex_init(&e->lock, NULL);
    pthread_cond_init(&e->cond, NULL);
    e->hnext = NULL;
    e->prev = e->next = NULL;
    return e;
}

int cache_entry_append(cache_entry_t *e, const char *buf, size_t n)
{
    if (!e)
        return -1;
    pthread_mutex_lock(&e->lock);
    size_t need = e->size + n;
    if (need + 1 > e->capacity)
    {
        size_t newcap = e->capacity ? e->capacity * 2 : 4096;
        while (newcap < need + 1)
            newcap *= 2;
        char *nd = realloc(e->data, newcap + 1);
        if (!nd)
        {
            pthread_mutex_unlock(&e->lock);
            return -1;
        }
        e->data = nd;
        e->capacity = newcap;
    }
    memcpy(e->data + e->size, buf, n);
    e->size += n;
    e->data[e->size] = 0;
    e->expire_at = time(NULL) + DEFAULT_TTL;
    pthread_cond_broadcast(&e->cond);
    pthread_mutex_unlock(&e->lock);
    return 0;
}

void cache_entry_mark_complete(cache_entry_t *e)
{
    pthread_mutex_lock(&e->lock);
    e->loading = 0;
    e->complete = 1;
    pthread_cond_broadcast(&e->cond);
    pthread_mutex_unlock(&e->lock);
}