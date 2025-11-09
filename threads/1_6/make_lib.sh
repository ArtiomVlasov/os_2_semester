#!/bin/bash

gcc -c -pthread my_thread.c -o my_thread.o
ar rcs libuthr.a my_thread.o

gcc -pthread -g main.c -L. -luthr -o a
./a