#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>
#include <time.h>
#include <pthread.h>

#define CACHE_HASH_SIZE 4096
#define DEFAULT_TTL 300
#define MAX_CACHE_TOTAL (100 * 1024 * 1024)
#define EVICT_BATCH 4

typedef struct cache_entry
{
    char *key;
    char *data;
    size_t size;
    size_t capacity;
    int loading;
    int complete;
    time_t last_access;
    time_t expire_at;
    struct cache_entry *hnext;
    struct cache_entry *prev;
    struct cache_entry *next;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} cache_entry_t;

typedef struct
{
    cache_entry_t *buckets[CACHE_HASH_SIZE];
    pthread_mutex_t lock;
    cache_entry_t *lru_head;
    cache_entry_t *lru_tail;
    size_t total_size;
} cache_t;

extern cache_t global_cache;

void cache_init(cache_t *c);
cache_entry_t *cache_find(cache_t *c, const char *key);
void cache_insert(cache_t *c, cache_entry_t *e);
void cache_evict_if_needed(cache_t *c);

cache_entry_t *cache_entry_create(const char *key, time_t ttl);
int cache_entry_append(cache_entry_t *e, const char *buf, size_t n);
void cache_entry_mark_complete(cache_entry_t *e);

void *cache_maintenance_thread(void *arg);

#endif