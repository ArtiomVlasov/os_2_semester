#define _GNU_SOURCE
#include <error.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *args) {
  *((int *)args) = 42;
  return args;
}

int main() {
  pthread_t tid;
  int err;
  int value = 0;
  int *value_pointer = &value;

  printf("main: value = %d\n", value);
  err = pthread_create(&tid, NULL, mythread, &value);
  if (err) {
    perror("main: error creating pthread");
    return -1;
  }

  err = pthread_join(tid, (void **)&value_pointer);
  if (err) {
    perror("main: error creating pthread");
    return -1;
  }

  printf("main: value after modification = %d\n", value);

  return 0;
}