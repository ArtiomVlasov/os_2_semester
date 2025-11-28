#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <list_length> <run_seconds>\n", argv[0]);
        printf("Example: %s 1000 10\n", argv[0]);
        return 1;
    }
    size_t n = (size_t)atol(argv[1]);
    int run_seconds = atoi(argv[2]);

    Storage *s = storage_create(n);
    if (!s) {
        fprintf(stderr, "Failed to create storage\n");
        return 2;
    }

    pthread_t t_asc, t_desc, t_eq, t_swap1, t_swap2, t_swap3, monitor;
    struct ThreadArg targ;
    targ.s = s;
    targ.thread_id = 0;

    pthread_create(&t_asc, NULL, ascending_thread, &targ);
    pthread_create(&t_desc, NULL, descending_thread, &targ);
    pthread_create(&t_eq, NULL, equal_thread, &targ);

    struct ThreadArg targ1 = {s, 0};
    struct ThreadArg targ2 = {s, 1};
    struct ThreadArg targ3 = {s, 2};
    pthread_create(&t_swap1, NULL, swap_thread, &targ1);
    pthread_create(&t_swap2, NULL, swap_thread, &targ2);
    pthread_create(&t_swap3, NULL, swap_thread, &targ3);

    pthread_create(&monitor, NULL, monitor_thread, &targ);
    sleep(run_seconds);
    stop_flag = 1;

    pthread_join(t_asc, NULL);
    pthread_join(t_desc, NULL);
    pthread_join(t_eq, NULL);
    pthread_join(t_swap1, NULL);
    pthread_join(t_swap2, NULL);
    pthread_join(t_swap3, NULL);
    pthread_join(monitor, NULL);

    printf("asc iterations: %lu\n", asc_iterations);
    printf("desc iterations: %lu\n", desc_iterations);
    printf("eq iterations: %lu\n", eq_iterations);
    printf("swap successes: %lu %lu %lu\n", swap_success[0], swap_success[1], swap_success[2]);

    storage_destroy(s);
    return 0;
}
