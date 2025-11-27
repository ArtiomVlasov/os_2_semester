#!/bin/bash

gcc -c -pthread queue.c -o queue.o
ar rcs libqueue.a queue.o

#gcc -pthread queue-example.c -L. -lqueue -o main
#./main

gcc -pthread queue-threads.c -L. -lqueue -o thr
./thr