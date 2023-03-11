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

#include "sha2.h"

typedef struct{
    int version;
    char previous_block_hash[32];
    char merkle_root[32];
    int timestamp;
    int difficulty;
    int nonce;
} BitcoinHeader;


// requires `sha2.c`, `sha2.h`
// computes double SHA - that is, SHA twice - for a given piece of input.
// params
//  *message: pointer to the data to be hashed
//  len: how long, in bytes, the input is
//  *digest: pointer to a 32-byte buffer to store the hash
// return
//  void
void dsha(const unsigned char* message, unsigned int len, unsigned char* digest){
    sha256(message, len, digest);
    sha256(digest, 32, digest);
}


// gives a random 32-byte hash.
// For simplicity, it just hashes a random integer.
// params
//  *digest: pointer to a 32-byte buffer to store the hash
// return
//  void
void get_random_hash(unsigned char* digest){
    int a=rand();
    dsha(&a, sizeof(int), digest);
}

// gives a random bitcoin header.
// params
//  *block: pointer to a header
//  difficulty: the difficulty to be written to the header
// return
//  void
void get_random_header(BitcoinHeader* block, int difficulty){
    block->version=2;
    get_random_hash(&(block->previous_block_hash));
    get_random_hash(&(block->merkle_root));
    block->timestamp=time(NULL);
    block->difficulty=difficulty;
    block->nonce=0;
}

// converts a "difficulty" value (a 32-bit integer) to a target hash that can be
// compared against
// params
//  d: the difficulty value
//  *target_storage: pointer to a 32-byte buffer to store the target
// return
//  void
void construct_target(int d, const char* target_storage){
    // first two hex digits in the difficulty is the exponent
    // last 6 are the coefficient
    // target = coefficient * 2^(8*(exponent-3))
    // which kinda means place the coefficient (exponent-3) bytes from the right, when written it big-endian
    // this implementation is likely flawed but should work for this purpose
    int coefficient=d%0x1000000;
    int exponent=d>>24;
    int placement_position=32-exponent;
    memset(target_storage, 0, 32);
    memset(target_storage+placement_position, coefficient>>16, 1);
    memset(target_storage+placement_position+1, (coefficient>>8)%0x100, 1);
    memset(target_storage+placement_position+2, coefficient%0x100, 1);
}

// checks whether a block is good.
// params
//  *block: pointer to a BitcoinHeader
//  *target: pointer to a 32-byte buffer containing the target
// return
//  1 or 0, to be interpreted as bool, whether the header's hash is below target
int is_good_block(BitcoinHeader* block, const char* target){
    char hash_storage[32];
    dsha(block, sizeof(BitcoinHeader), &hash_storage);
    if (memcmp(hash_storage, target, 32)<0){
        return 1;
    }else{
        return 0;
    }
}

// mines a single header
// params
//  *block: pointer to a block to be mined
//  *target: pointer to a expanded target
// return
//  void
void mine_single_block(BitcoinHeader* block, const char* target){
    int i=0;
    for(; i<=2147483647; i++){
        block->nonce=i;
        if(is_good_block(block, target)){
            printf("CHILD PROCESS: nonce found %d\n", i);
            break;
        }
        
    }
}

typedef struct{
    int result_found;
    BitcoinHeader block;
    char* target;
    int no_more_jobs;
}SharedData;

// The function that child processes should run.
// As of the template, the parameters and return values are documented below,
// but feel free to modify it to suit your needs.
// params
//  id: a numerical identifier to this process
//  *sd: a pointer to shared data
// return
//  void
void process_miner(int id, SharedData* sd){
    printf("CHILD %d: spawned\n",id);
    // prepare semaphore references
    sem_t* issue_job_sync_sem=sem_open("/issuejob", O_RDWR);
    sem_t* job_end_sync_sem=sem_open("/jobend", O_RDWR);
    sem_t* result_found_mutex=sem_open("/resultfound", O_RDWR);
    
    // prepare vars
    BitcoinHeader working_block;
    
    // main loop
    while(1){
        // wait for parent to issue job and release lock
        sem_wait(issue_job_sync_sem);
        // check `sd` for no-more-jobs signal
        if(sd->no_more_jobs){
            printf("CHILD %d: No more job flag raised, breaking out of main loop\n", id);
            break;
        }
        
        // make a copy of the block so that it doesn't mess with other processes
        working_block=sd->block;
        
        for(int i=0; i<=2147483647; i++){
            if(sd->result_found){
                printf("CHILD %d: Someone found result; breaking at i=%d\n", id, i);
                break;
            }
            working_block.nonce=i;
            if(is_good_block(&working_block, sd->target)){
                sem_wait(result_found_mutex);
                // *actually* make sure no one beat me to it
                if(sd->result_found){
                    printf("CHILD %d: I found a nonce, but someone beat me to it in writing it to the shared memory.\n",id);
                }else{
                    sd->result_found=1;
                    sd->block.nonce=i;
                    printf("CHILD %d: found a valid nonce %d\n", id, i);
                }
                sem_post(result_found_mutex);
                break;
            }
        }
        
        // tell parent that I'm done
        sem_post(job_end_sync_sem);
    }
    
}


int main(){
    // seed the random number generator
    srand(time(NULL));
    
    int difficulty=0x1f03a30c;
    char target[32];
    construct_target(difficulty, &target);
    
    // synchronization
    int num_processes=5;
    int num_tasks=10; // so that no endless loop is made
    
    // prepare shared memory
    int fd=shm_open("/minejobs", O_CREAT|O_RDWR, 0666);
    ftruncate(fd, sizeof(SharedData));
    SharedData* sd=mmap(NULL, sizeof(SharedData), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    sd->result_found=0;
    sd->no_more_jobs=0;
    sd->target=&target;
    
    // prepare semaphores
    sem_t* issue_job_sync_sem=sem_open("/issuejob",O_CREAT|O_RDWR, 0666, 0);
    sem_t* job_end_sync_sem=sem_open("/jobend",O_CREAT|O_RDWR, 0666, 0);
    sem_t* result_found_mutex=sem_open("/resultfound",O_CREAT|O_RDWR, 0666, 1);
    
    pid_t pid;
    for(int fork_num=0; fork_num<num_processes; fork_num++){
        pid=fork();
        if(pid==0){
            process_miner(fork_num, sd);
            exit(0);
        }
    }
    
    for(int task_num=0; task_num<num_tasks; task_num++){
        // prepare task
        get_random_header(&(sd->block), difficulty);
        sd->result_found=0;
        
        // release children
        for(int fork_num=0; fork_num<num_processes; fork_num++){
            sem_post(issue_job_sync_sem);
        }
        
        // wait for children to finish
        for(int fork_num=0; fork_num<num_processes; fork_num++){
            sem_wait(job_end_sync_sem);
        }
        
        // we should now have a result we could print
        printf("PARENT: A result was found with nonce=%d\n",sd->block.nonce);
    }
    
    sd->no_more_jobs=1;
    for(int fork_num=0; fork_num<num_processes; fork_num++){
        sem_post(issue_job_sync_sem);
    }
    
    for(int fork_num=0; fork_num<num_processes; fork_num++){
        wait(NULL);
    }
    
    return 0;
    
}



























