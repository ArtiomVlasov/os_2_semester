#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *args)
{
    unsigned long long cntr = 0;
    while (1)
    {
        ++cntr;
        // sleep(1);
    }
    return NULL;
}

int main()
{
    pthread_t tid;
    int err;

    err = pthread_create(&tid, NULL, mythread, NULL);
    if (err)
    {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    printf("main: thread created\n");

    sleep(3);

    err = pthread_cancel(tid);
    if (err)
    {
        perror("main: error canceling thread");
        return -1;
    }

    printf("main: sent thread cancel request\n");

    int *result;
    err = pthread_join(tid, (void **)&result);
    if (err)
    {
        perror("main: error joining thread");
        return -1;
    }

    printf("main: pthread canceled: %d\n", result == PTHREAD_CANCELED);
    return 0;
}