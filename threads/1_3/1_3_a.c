#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct
{
    int a;
    char *b;
} my_struct;

void *mythread(void *args)
{
    my_struct *s = (my_struct *)args;
    printf("thread: %d; %s\n", s->a, s->b);
    return NULL;
}

int main()
{

    my_struct s = {
        10, "HELLO, WORLD!!!\n"};

    printf("main: struct initialized with %d, %s\n", s.a, s.b);

    pthread_t tid;
    int err;

    err = pthread_create(&tid, NULL, mythread, &s);
    if (err)
    {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_join(tid, NULL);
    if (err)
    {
        perror("main: error joining thread");
        return -1;
    }

    return 0;
}