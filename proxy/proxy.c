#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include "cache.h"
#include "net.h"
#include "log.h"

#define MAXLINE 8192

typedef struct
{
    cache_entry_t *entry;
    char host[512];
    char path[1536];
    int port;
} fetcher_arg_t;

void *fetcher_thread(void *arg)
{
    fetcher_arg_t *fa = arg;
    cache_entry_t *entry = fa->entry;
    log_msg("Fetcher started for key=%s host=%s port=%d path=%s", entry->key, fa->host, fa->port, fa->path);

    int s = connect_to_host(fa->host, fa->port);
    if (s < 0)
    {
        log_msg("Failed to connect to origin %s:%d", fa->host, fa->port);
        cache_entry_mark_complete(entry);
        free(fa);
        return NULL;
    }

    char req[4096];
    int rl = snprintf(req, sizeof(req),
                      "GET %s HTTP/1.0\r\n"
                      "Host: %s\r\n"
                      "Connection: close\r\n"
                      "\r\n",
                      fa->path, fa->host);
    ssize_t sent = 0;
    while (sent < rl)
    {
        ssize_t w = send(s, req + sent, rl - sent, 0);
        if (w <= 0)
        {
            log_msg("Failed to send request to origin");
            close(s);
            cache_entry_mark_complete(entry);
            free(fa);
            return NULL;
        }
        sent += w;
    }

    char buf[8192];
    ssize_t n;
    while ((n = recv(s, buf, sizeof(buf), 0)) > 0)
    {
        if (cache_entry_append(entry, buf, (size_t)n) != 0)
        {
            log_msg("Failed to append to cache entry for key=%s", entry->key);
            break;
        }
        pthread_mutex_lock(&global_cache.lock);
        global_cache.total_size += n;
        // promote_lru(&global_cache, entry);
        pthread_mutex_unlock(&global_cache.lock);
    }
    if (n == 0)
    {
        log_msg("Fetcher finished reading origin for key=%s", entry->key);
    }
    else
    {
        log_msg("Fetcher recv error for key=%s: %s", entry->key, strerror(errno));
    }
    close(s);
    cache_entry_mark_complete(entry);

    pthread_mutex_lock(&global_cache.lock);
    cache_evict_if_needed(&global_cache);
    pthread_mutex_unlock(&global_cache.lock);

    free(fa);
    return NULL;
}

void *client_thread(void *arg)
{
    int client_fd = (int)(intptr_t)arg;
    char line[MAXLINE];
    ssize_t r = read_line(client_fd, line, sizeof(line));
    if (r <= 0)
    {
        close(client_fd);
        return NULL;
    }
    log_msg("Client request first line: %s", line);
    request_info_t ri;
    extract_from_request(client_fd, line, &ri);
    if (ri.host[0] == 0)
    {
        log_msg("Cannot extract host from request; closing");
        close(client_fd);
        return NULL;
    }
    char key[4096];
    snprintf(key, sizeof(key), "http://%s:%d%s", ri.host, ri.port, ri.path);

    cache_entry_t *entry = NULL;
    pthread_mutex_lock(&global_cache.lock);
    entry = cache_find(&global_cache, key);
    if (entry)
    {
        entry->last_access = time(NULL);
        // promote_lru(&global_cache, entry);
        log_msg("Cache hit for key=%s (loading=%d complete=%d size=%zu)", key, entry->loading, entry->complete, entry->size);
    }
    else
    {
        entry = cache_entry_create(key, DEFAULT_TTL);
        if (!entry)
        {
            pthread_mutex_unlock(&global_cache.lock);
            log_msg("Failed to create cache entry");
            close(client_fd);
            return NULL;
        }
        cache_insert(&global_cache, entry);
        log_msg("Cache miss. Created entry & launching fetcher for key=%s", key);
        fetcher_arg_t *fa = malloc(sizeof(*fa));
        fa->entry = entry;
        strncpy(fa->host, ri.host, sizeof(fa->host) - 1);
        strncpy(fa->path, ri.path, sizeof(fa->path) - 1);
        fa->port = ri.port;
        pthread_t tid;
        if (pthread_create(&tid, NULL, fetcher_thread, fa) != 0)
        {
            log_msg("Failed to create fetcher thread");
            cache_entry_mark_complete(entry);
        }
        else
        {
            pthread_detach(tid);
        }
    }
    pthread_mutex_unlock(&global_cache.lock);

    size_t send_offset = 0;
    int done = 0;
    while (!done)
    {
        pthread_mutex_lock(&entry->lock);
        while (send_offset >= entry->size && !entry->complete)
        {
            pthread_cond_wait(&entry->cond, &entry->lock);
        }
        size_t available = entry->size - send_offset;
        if (available > 0)
        {
            size_t tosend = available;
            ssize_t sent = send(client_fd, entry->data + send_offset, tosend, 0);
            if (sent <= 0)
            {
                pthread_mutex_unlock(&entry->lock);
                log_msg("Client disconnected while sending");
                close(client_fd);
                return NULL;
            }
            send_offset += (size_t)sent;
            entry->last_access = time(NULL);
            pthread_mutex_unlock(&entry->lock);
            continue;
        }
        if (entry->complete && send_offset >= entry->size)
        {
            done = 1;
        }
        pthread_mutex_unlock(&entry->lock);
    }

    log_msg("Finished serving client for key=%s sent=%zu bytes", entry->key, send_offset);
    close(client_fd);
    return NULL;
}
