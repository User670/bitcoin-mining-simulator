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
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

#include <sha2.h>
#include <bitcoin_utils.h>
#include <data_utils.h>
#include <debug_utils.h>
#include <custom_errors.h>

#define CP(x) printf("Checkpoint %d\n",x);

#define NUM_PROCESSES 5
#define NUM_THREADS 5
#define SHARED_BUF_CHAIN_SIZE 1048576 //1MB
#define SHARED_BUF_BLOCK_SIZE 32768   //32KB

int main(){
    printf("%d\n",sizeof(BitcoinBlockv4));
}