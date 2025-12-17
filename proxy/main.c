#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "cache.h"
#include "net.h"
#include "proxy.h"
#include "log.h"

#define LISTEN_PORT 80
#define BACKLOG 128


int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    cache_init(&global_cache);

    pthread_t maint;
    pthread_create(&maint, NULL, cache_maintenance_thread, &global_cache);
    pthread_detach(maint);

    int listen_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("socket");
        return 1;
    }
    int off = 0;
    setsockopt(listen_fd, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));
    int on = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(LISTEN_PORT);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(listen_fd);
        return 1;
    }
    if (listen(listen_fd, BACKLOG) < 0)
    {
        perror("listen");
        close(listen_fd);
        return 1;
    }
    log_msg("HTTP proxy listening on port %d (use sudo for port <1024). Max cache total = %d bytes", LISTEN_PORT, MAX_CACHE_TOTAL);

    for (;;)
    {
        struct sockaddr_storage cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int cli = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
        if (cli < 0)
        {
            if (errno == EINTR)
                continue;
            perror("accept");
            break;
        }
        pthread_t tid;
        int *pfd = malloc(sizeof(int));
        *pfd = cli;
        if (pthread_create(&tid, NULL, client_thread, (void *)(intptr_t)cli) != 0)
        {
            log_msg("Failed to create thread for client");
            close(cli);
            free(pfd);
            continue;
        }
        pthread_detach(tid);
    }

    close(listen_fd);
    return 0;
}
