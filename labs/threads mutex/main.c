//chatGPT
// Goal is to see if `pthread_mutex`-series of functions work
// man-pages doesn't have documentation on Linux inplementation, only POSIX
// specification
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define NUM_THREADS 5
int count = 0;
pthread_mutex_t lock;
void *increment_count(void *thread_id) {
    int i;
    for (i = 0; i < 1000000; i++) {
        pthread_mutex_lock(&lock);
        count++;
        pthread_mutex_unlock(&lock);
    }
    pthread_exit(NULL);
}
int main() {
    pthread_t threads[NUM_THREADS];
    int rc, i;
    // initialize mutex
    pthread_mutex_init(&lock, NULL);
    // create threads
    for (i = 0; i < NUM_THREADS; i++) {
        rc = pthread_create(&threads[i], NULL, increment_count, (void *)&i);
        if (rc) {
            printf("Error creating thread %d\n", rc);
            exit(-1);
        }
    }
    // wait for threads to complete
    for (i = 0; i < NUM_THREADS; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc) {
            printf("Error joining thread %d\n", rc);
            exit(-1);
        }
    }
    // destroy mutex
    pthread_mutex_destroy(&lock);
    printf("Final count: %d\n", count);
    return 0;
}