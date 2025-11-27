#define _GNU_SOURCE
#include "my_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
void *my_thread_function(void *arg)
{
    printf("mythread [%d %d %d]\n", getpid(), getppid(), gettid());
    printf("Поток запущен с аргументом: %s\n", (char *)arg);
    sleep(2);
    int *res = malloc(sizeof(int));
    *res = 5;
    return res;
}

int main()
{
    mythread_t thread;
    char *arg = "Hi!";
    printf("         [PID  PPID  TID]\n");

    printf("main     [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    if (mythread_create(&thread, my_thread_function, arg) == -1)
    {
        printf("Cannot create thread");
        return 1;
    }
    sleep(5);
    // void *retval;
    mythread_detach(&thread);
    // int result = *(int *)retval;  
    // free(retval);

    // printf("Thread finished with arg %d\n", result);

    return 0;
}