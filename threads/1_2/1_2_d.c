#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg) {
  // pthread_detach(pthread_self()); 
  printf("%lu\n", pthread_self());
  return NULL;
}

int main() {
  pthread_t tid;
  int err;
  pthread_attr_t attr;

  pthread_attr_init(&attr);

  err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (err) {
    perror("main: setting attr");
    return -1;
  }

  while (1) {
    err = pthread_create(&tid, &attr, mythread, NULL);
    // err = pthread_create(&tid, NULL, mythread, NULL);
    if (err) {
      perror("main: error creating thread\n");
      return -1;
    }
  }
  pthread_attr_destroy(&attr);


  return 0;
}