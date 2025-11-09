#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdbool.h>
#include <signal.h>

const int THREADS_NUMBER = 3;
const int BLOCK_ALL_THREAD = 0;
const int SIGINT_THREAD = 1;
const int SIGWAIT_THREAD = 2;

void *thread_block_all(void *args)
{
    sigset_t set;
    int err;

    err = sigfillset(&set);
    if (err != 0)
    {
        perror("sigfillset");
        pthread_exit(NULL);
    }

    err = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (err != 0)
    {
        perror("pthread_sigmask");
        pthread_exit(NULL);
    }
    sleep(7);
    printf("Thread block all: all signals blocked\n");

    while (1)
        sleep(1);

    pthread_exit(NULL);
}

void sigint_handler(int s)
{
    write(1, "Caught SIGINT\n", 14);
}

void *thread_SIGINT(void *args)
{
    int err;

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = sigint_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    err = sigaction(SIGINT, &act, NULL);
    if (err != 0)
    {
        printf("Thread SIGINT: Failed 'sigaction()': %s\n", strerror(err));
        pthread_exit(NULL);
    }

    // int counter = 0;
    while (true)
    {
        // counter++;
        sleep(1);
    }

    pthread_exit(NULL);
}

void *thread_SIGWAIT(void *args)
{
    int err;

    sigset_t set;
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = sigint_handler;
    err = sigemptyset(&set);
    err = sigaction(SIGQUIT, &act, NULL);

    if (err != 0)
    {
        printf("Failed 'sigemptyset': %s\n", strerror(err));
        pthread_exit(NULL);
    }

    err = sigaddset(&set, SIGQUIT);
    if (err != 0)
    {
        printf("Failed 'sigaddset(.., SIGQUIT)': %s\n", strerror(err));
        pthread_exit(NULL);
    }

    err = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (err != 0)
    {
        printf("Failed blocking sigmask: %s\n", strerror(err));
        pthread_exit(NULL);
    }

    int sig;
    err = sigwait(&set, &sig);
    if (err != 0)
    {
        printf("Failed 'sigwait': %s\n", strerror(err));
        pthread_exit(NULL);
    }

    printf("Thread SIGWAIT: Caught SIGQUIT\n");
    pthread_exit(NULL);
}

int main()
{
    pthread_t threads[THREADS_NUMBER];
    int err;

    err = pthread_create(&threads[BLOCK_ALL_THREAD], NULL, thread_block_all, NULL);
    if (err != 0)
    {
        printf("Failed to create block all thread: %s\n", strerror(err));
        return 0;
    }

    err = pthread_create(&threads[SIGINT_THREAD], NULL, thread_SIGINT, NULL);
    if (err != 0)
    {
        printf("Failed to create thread SIGINT-handler: %s\n", strerror(err));
        return 0;
    }

    err = pthread_create(&threads[SIGWAIT_THREAD], NULL, thread_SIGWAIT, NULL);
    if (err != 0)
    {
        printf("Failed to create sigwait thread: %s\n", strerror(err));
        return 0;
    }

    sleep(5);

    err = pthread_kill(threads[BLOCK_ALL_THREAD], SIGABRT);
    if (err != 0)
        printf("pthread_kill: %s\n", strerror(err));
    else
        printf("main: Sent SIGABRT\n");

    sleep(1);

    err = pthread_kill(threads[BLOCK_ALL_THREAD], SIGINT);
    if (err != 0)
        printf("pthread_kill: %s\n", strerror(err));
    else
        printf("main: Sent SIGINT\n");
    sleep(1);

    err = pthread_kill(threads[SIGINT_THREAD], SIGINT);
    if (err != 0)
    {
        printf("Sigint thread error: %s...\n", strerror(err));
        return 0;
    }
    printf("\nmain: Sent SIGINT\n");
    sleep(5);

    err = pthread_kill(threads[SIGWAIT_THREAD], SIGQUIT);
    if (err != 0)
    {
        printf("Sigquit thread error: %s...\n", strerror(err));
        return 0;
    }
    printf("\nmain: Sent SIGQUIT\n");

    for (int i = 0; i < THREADS_NUMBER; i++)
    {
        void *retval;
        pthread_cancel(threads[i]);
        err = pthread_join(threads[i], &retval);
        if (err != 0)
        {
            printf("Failed joining thread: %s\n", strerror(err));
            return 0;
        }
    }

    return 0;
}