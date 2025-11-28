#include "uthread.h"
#include <stdio.h>
#include <unistd.h>

void* uthread_func1(void* arg) {
  uthread_sleep(1000);   
  for (int i = 0; i < 3; i++) {
        printf("[T1] %d\n", i);
        uthread_sleep(1000);     
    }
    printf("[T1] finished\n");
    return NULL;
}

void* uthread_func2(void* arg) {
    for (int i = 0; i < 3; i++) {
        printf("[T2] %d\n", i);
        uthread_sleep(1000);
    }
    printf("[T2] finished\n");
    return NULL;
}

void* uthread_func3(void* arg) {
    for (int i = 0; i < 3; i++) {
        printf("[T3] %d\n", i);
        schedule();

    }
    printf("[T3] finished\n");
    return NULL;
}

int main(void) {

    printf("[MAIN] init scheduler\n");
    scheduler_init();

    uthread_t tid1, tid2, tid3;

    if (uthread_create(&tid1, uthread_func1, NULL) != 0) {
        printf("ERROR: uthread_create(t1) failed\n");
        return -1;
    }

    if (uthread_create(&tid2, uthread_func2, NULL) != 0) {
        printf("ERROR: uthread_create(t2) failed\n");
        return -1;
    }

    if (uthread_create(&tid3, uthread_func3, NULL) != 0) {
        printf("ERROR: uthread_create(t3) failed\n");
        return -1;
    }

    printf("[MAIN] start round-robin\n");

    for (int i = 0; i < 3; i++) {
        printf("[MAIN] %d\n", i);
        uthread_sleep(2000);
    }
    // schedule();
    printf("[MAIN] finished\n");

    scheduler_destroy();
    return 0;
}