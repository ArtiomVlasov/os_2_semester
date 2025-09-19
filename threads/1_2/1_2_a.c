#define _GNU_SOURCE
#include <error.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg) {
  printf("thread: THREAD WAS CREATED\n");
  printf("thread: return\n");
  sleep(10);
  return NULL;
}

int main() {
  printf("main: started\n");
  pthread_t tid;
  int err;

  err = pthread_create(&tid, NULL, mythread, NULL);
  if (err) {
    perror("main: pthread creating error\n");
    return -1;
  }

  printf("main: join\n");

  err = pthread_join(tid, NULL);
  if (err) {
    perror("main: pthread joining error\n");
    return -1;
  }

  printf("main: join finished successfully\n");

  return 0;
}