#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

typedef struct
{
    int a;
    char *b;
} my_struct;

void *mythread(void *args)
{
    my_struct *s = (my_struct *)args;
    printf("thread: %d; %s", s->a, s->b);
    return NULL;
}

int main()
{

    my_struct *s = malloc(sizeof(my_struct));
    if (!s)
    {
        perror("malloc");
        return -1;
    }

    s->a = 10;
    s->b = "HELLO, WORLD!!!\n";

    printf("main: struct initialized with %d, %s", s->a, s->b);

    pthread_t tid;
    pthread_attr_t attr;
    int err;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    err = pthread_create(&tid, &attr, mythread, s);
    pthread_attr_destroy(&attr);

    if (err)
    {
        perror("main: error during thread creation");
        free(s);
        return -1;
    }
    sleep(1);
    free(s);

    return 0;
}