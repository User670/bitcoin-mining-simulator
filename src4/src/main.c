#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>

#include <sha2.h>
#include <bitcoin_utils.h>
#include <data_utils.h>
#include <debug_utils.h>

int main(){
    int difficulty=0x1f03a30c;
    char target[32];
    construct_target(difficulty, &target);
    
    struct timespec start, end;
    
    BitcoinHeader a;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int i=0; i<1000000; i++){
        a.nonce=i;
        is_good_block(&a, target);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + end.tv_nsec - start.tv_nsec;
    printf("Elapsed time: %'lld ns\n", elapsed_ns);
}

/*
#include <stdio.h>
#include <time.h>

int main() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start); // get start time
    // code to be timed goes here
    clock_gettime(CLOCK_MONOTONIC, &end); // get end time
    long long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + end.tv_nsec - start.tv_nsec; // calculate elapsed time in nanoseconds
    printf("Elapsed time: %lld ns\n", elapsed_ns);
    return 0;
}
*/