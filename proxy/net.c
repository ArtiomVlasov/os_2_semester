#define _GNU_SOURCE
#include "net.h"
#include "log.h"
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define MAXLINE 8192


ssize_t read_line(int fd, char *buf, size_t maxlen)
{
    size_t i = 0;
    char c;
    ssize_t n;
    while (i < maxlen - 1)
    {
        n = recv(fd, &c, 1, 0);
        if (n <= 0)
            return (n == 0 && i > 0) ? (ssize_t)i : n;
        buf[i++] = c;
        if (c == '\n')
            break;
    }
    buf[i] = 0;
    return (ssize_t)i;
}

void extract_from_request(int client_fd, const char *first_line, request_info_t *ri)
{
    memset(ri, 0, sizeof(*ri));
    ri->port = 80;
    char method[16], url[2048], version[64];
    if (sscanf(first_line, "%15s %2047s %63s", method, url, version) != 3)
    {
        log_msg("Malformed request line: %s", first_line);
        return;
    }
    if (strncmp(url, "http://", 7) == 0)
    {
        const char *p = url + 7;
        const char *slash = strchr(p, '/');
        if (slash)
        {
            size_t hostlen = slash - p;
            char hostport[512];
            if (hostlen >= sizeof(hostport))
                hostlen = sizeof(hostport) - 1;
            strncpy(hostport, p, hostlen);
            hostport[hostlen] = 0;
            char *col = strchr(hostport, ':');
            if (col)
            {
                *col = 0;
                ri->port = atoi(col + 1);
            }
            strncpy(ri->host, hostport, sizeof(ri->host) - 1);
            snprintf(ri->path, sizeof(ri->path), "%s", slash);
            snprintf(ri->url, sizeof(ri->url), "%s", url);
            return;
        }
        else
        {
            strncpy(ri->host, p, sizeof(ri->host) - 1);
            strcpy(ri->path, "/");
            snprintf(ri->url, sizeof(ri->url), "%s", url);
            return;
        }
    }
    else
    {
        strncpy(ri->path, url, sizeof(ri->path) - 1);
        char line[MAXLINE];
        char hostbuf[512] = {0};
        while (1)
        {
            ssize_t r = read_line(client_fd, line, sizeof(line));
            if (r <= 0)
                break;
            if (strcmp(line, "\r\n") == 0 || strcmp(line, "\n") == 0)
                break;
            if (strncasecmp(line, "Host:", 5) == 0)
            {
                char *p = line + 5;
                while (*p == ' ' || *p == '\t')
                    p++;
                char *e = p + strlen(p) - 1;
                while (e >= p && (*e == '\r' || *e == '\n'))
                {
                    *e = 0;
                    e--;
                }
                strncpy(hostbuf, p, sizeof(hostbuf) - 1);
            }
        }
        if (hostbuf[0])
        {
            char *col = strchr(hostbuf, ':');
            if (col)
            {
                *col = 0;
                ri->port = atoi(col + 1);
            }
            strncpy(ri->host, hostbuf, sizeof(ri->host) - 1);
        }
        else
        {
            log_msg("No Host header found for relative URL");
        }
        snprintf(ri->url, sizeof(ri->url),"http://%.500s%.1500s", ri->host, ri->path);
    }
}

int connect_to_host(const char *host, int port)
{
    struct addrinfo hints, *res, *rp;
    char portbuf[16];
    snprintf(portbuf, sizeof(portbuf), "%d", port);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, portbuf, &hints, &res) != 0)
    {
        log_msg("getaddrinfo failed for %s:%d", host, port);
        return -1;
    }
    int s = -1;
    for (rp = res; rp; rp = rp->ai_next)
    {
        s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s < 0)
            continue;
        if (connect(s, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        close(s);
        s = -1;
    }
    freeaddrinfo(res);
    return s;
}

