//chatGPT
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define NUM_THREADS 5
void *print_hello(void *thread_id) {
    int id = *((int*)thread_id);
    printf("Hello from thread %d\n", id);
    pthread_exit(NULL);
}
int main() {
    pthread_t threads[NUM_THREADS];
    int thread_args[NUM_THREADS];
    int rc, i;
    // create threads
    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        rc = pthread_create(&threads[i], NULL, print_hello, (void *)&thread_args[i]);
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
    return 0;
}