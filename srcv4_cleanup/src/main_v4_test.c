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

int main(){
    //make a chain
    int rv;
    BitcoinBlockv4 b1, b2, b3;
    get_dummy_genesis_block_v4(&b1);
    get_dummy_genesis_block_v4(&b2);
    get_dummy_genesis_block_v4(&b3);
    
    char h1[32];
    dsha(&(b1.header), sizeof(BitcoinHeader), h1);
    char name1[100];
    construct_shm_name(h1, name1);
    
    CP(1)
    
    int fd;
    BitcoinBlockv4* mblock;
    
    fd=shm_open(name1, O_CREAT|O_RDWR, 0666);
    ftruncate(fd, sizeof(BitcoinBlockv4));
    mblock=mmap(NULL, sizeof(BitcoinBlockv4), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    memcpy(mblock, &b1, sizeof(BitcoinBlockv4));
    
    close(fd);
    //shm_unlink(name1);
    munmap(mblock,sizeof(BitcoinBlockv4));
    
    CP(2)
    
    randomize_block_transactions_v4(&b2);
    randomize_block_transactions_v4(&b3);
    char name2[100];
    char name3[100];
    rv=attach_block_v4(name1, &b2, name2);
    if(rv<0)perror_custom(rv, "attach block 2");
    
    CP(3)
    
    fd=shm_open(name2, O_CREAT|O_RDWR, 0666);
    ftruncate(fd, sizeof(BitcoinBlockv4));
    mblock=mmap(NULL, sizeof(BitcoinBlockv4), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    memcpy(mblock, &b2, sizeof(BitcoinBlockv4));
    
    close(fd);
    //shm_unlink(name2);
    munmap(mblock,sizeof(BitcoinBlockv4));
    
    CP(4)
    
    rv=attach_block_v4(name1, &b3, name3);
    if(rv<0)perror_custom(rv, "attach block 3");
    
    CP(5)
    
    fd=shm_open(name3, O_CREAT|O_RDWR, 0666);
    ftruncate(fd, sizeof(BitcoinBlockv4));
    mblock=mmap(NULL, sizeof(BitcoinBlockv4), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    memcpy(mblock, &b3, sizeof(BitcoinBlockv4));
    
    close(fd);
    //shm_unlink(name3);
    munmap(mblock,sizeof(BitcoinBlockv4));
    CP(6)
    
    printf("Total number of blocks in the chain: %d\n",get_blockchain_length(name1));
    
}