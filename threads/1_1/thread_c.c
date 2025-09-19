#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define THREADS_AMOUNT 5

int global = 1;

void *mythread(void *arg)
{
    printf("mythread [%d %d %d %lu]: Hello from mythread!\n", getpid(), getppid(), gettid(), pthread_self());
    int local = 2;
    static int local_static = 3;
    const int constanta = 4;
    printf("mythread [%d %d %d %lu]: \nVaribles: [local: %p, local_static: %p, constanta: %p, global: %p]\n",
           getpid(), getppid(), gettid(), pthread_self(), &local, &local_static, &constanta, &global);
    // sleep(20);
    return NULL;
}

int main()
{
    pthread_t tids[THREADS_AMOUNT];
    int err;

    printf("main [%d %d %d %lu]: Hello from main!\n", getpid(), getppid(),
           gettid(), pthread_self());

    for (int i = 0; i < THREADS_AMOUNT; ++i)
    {
        err = pthread_create(&(tids[i]), NULL, mythread, NULL);
        if (err)
        {
            printf("main: pthread_create() failed: %s\n", strerror(err));
            return -1;
        }
        printf("main [%d %d %d %lu]: Created thread [%lu]!\n", getpid(), getppid(), gettid(), pthread_self(), tids[i]);
    }

    for (int i = 0; i < THREADS_AMOUNT; ++i)
    {
        err = pthread_join(tids[i], NULL);
        if (err)
        {
            perror("main: pthread joining error");
            return -1;
        }
    }

    sleep(2000);

    return 0;
}