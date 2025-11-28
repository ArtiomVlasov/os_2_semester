#ifndef LIB_H
#define LIB_H

#include <pthread.h>
#include <stdint.h>

#define MAX_STR 100

typedef struct Node {
    char value[MAX_STR];
    struct Node* next;
    pthread_mutex_t sync;
} Node;

typedef struct Storage {
    Node *sentinel; 
    size_t length; 
} Storage;

struct ThreadArg {
    Storage *s;
    int thread_id; 
};

Storage* storage_create(size_t n);
void storage_destroy(Storage* s);


void* ascending_thread(void* arg);
void* descending_thread(void* arg);
void* equal_thread(void* arg);
void* swap_thread(void* arg);
void *monitor_thread(void *arg);

extern uint64_t asc_iterations;
extern uint64_t desc_iterations;
extern uint64_t eq_iterations;
extern uint64_t swap_success[3];

extern volatile int stop_flag;

#endif 
