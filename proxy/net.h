#ifndef NET_H
#define NET_H

#include <stddef.h>
#include <stdio.h>
#include "cache.h"

ssize_t read_line(int fd, char *buf, size_t maxlen);
int connect_to_host(const char *host, int port);

typedef struct
{
    char url[2048];
    char host[512];
    char path[1536];
    int port;
} request_info_t;

void extract_from_request(int client_fd, const char *first_line, request_info_t *ri);

#endif