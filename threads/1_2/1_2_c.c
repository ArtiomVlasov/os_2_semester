#define _GNU_SOURCE
#include <error.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *args)
{
    char *str = "Hello world";
    return str;
}

int main()
{
    pthread_t tid;
    int err;
    char *str;

    err = pthread_create(&tid, NULL, mythread, NULL);
    if (err)
    {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_join(tid, (void **)&str);
    if (err)
    {
        perror("main: error creating pthread");
        return -1;
    }

    printf("main: %s\n", str);

    return 0;
}