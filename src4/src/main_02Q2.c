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
#include <signal.h>
#include <fcntl.h>

#include <sha2.h>
#include <bitcoin_utils.h>
#include <data_utils.h>
#include <debug_utils.h>

#define NUM_PROCESSES 5
#define MAX_BLOCKCHAIN_HEIGHT 10

typedef struct{
    int chain_height;
    BitcoinHeader chain[MAX_BLOCKCHAIN_HEIGHT];
}SharedData1;


void sig_hand(int sig){
    if(sig==SIGINT){
        printf("SIGINT (Ctrl+C) received. Cleaning up...\n");
    }else{
        printf("Another signal (%d) received by the signal handler.\n",sig);
        printf("Cleaning up anyways, but check for bugs in the code.\n");
    }
    shm_unlink("/minechain");
    sem_unlink("/minechainsync");
    sem_unlink("/minechainmutex");
    printf("Done. Exiting...\n");
    exit(0);
}

int main(){
    // seed the random number generator
    srand(time(NULL));
    
    // C warm up: brute force one block
    int difficulty=0x1f03a30c;
    char target[32];
    construct_target(difficulty, &target);
    
    // signal handler, because we want to handle Ctrl+C (SIGINT)
    sigset_t set;
    struct sigaction sigact;
    sigfillset(&set);
    sigact.sa_handler = sig_hand;
    sigact.sa_mask = set;
    sigact.sa_flags = 0;
    sigaction(SIGINT,&sigact,0);
    
    // synchronization, or an attempt of it
    
    // prepare shared memory
    int fd;
    int is_first_create=0;
    fd=shm_open("/minechain", O_CREAT|O_EXCL|O_RDWR, 0666);
    if(fd==-1){
        fd=shm_open("/minechain", O_CREAT|O_RDWR, 0666);
    }else{
        is_first_create=1;
    }
    ftruncate(fd, sizeof(SharedData1));
    SharedData1* sd=mmap(NULL, sizeof(SharedData1), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    
    sem_t* start_sync_sem=sem_open("/minechainsync", O_CREAT|O_RDWR, 0666, 0);
    sem_t* chain_access_mutex=sem_open("/minechainmutex", O_CREAT|O_RDWR, 0666, 1);
    
    int process_count;
    
    if(is_first_create){
        // initialize the shared data
        sd->chain_height=0;
        get_random_header(&(sd->chain[0]), difficulty);
        
        printf("Shared memory didn't already exist. Assumed this is the first instance, and opened shared memory.\n");
        printf("Open more instances from other terminal windows.\n");
        printf("After that, type the number of instances you opened (including this one).\n");
        printf("Type 0 or a negative number to exit.\n");
        printf("Number of instances? > ");
        scanf("%d", &process_count);
        if(process_count<=0){
            printf("Cleaning up. If you have another instance open, cleanup might not be complete; send SIGINT (Ctrl+C) to other instances to clean up.\n");
            shm_unlink("/minechain");
            sem_unlink("/minechainsync");
            sem_unlink("/minechainmutex");
            printf("Done. Exiting...\n");
            exit(0);
        }else{
            printf("posting semaphore %d times...\n", process_count-1);
            for(int i=0; i<process_count-1; i++){
                sem_post(start_sync_sem);
            }
        }
    }else{
        printf("Shared memory already exists. Waiting for start signal from the first instance.\n");
        printf("If there is no such instance, fix by performing the following actions:\n");
        printf(" - close all but one instances,\n");
        printf(" - send a SIGINT (Ctrl+C) to the last instance.\n");
        printf("The PID of this instance is %d. If you need to force kill this instance, use `kill -9 %d`.\n", getpid(), getpid());
        printf("------------------------\n");
        sem_wait(start_sync_sem);
    }
    
    
    int working_on_height;
    BitcoinHeader working_on_block;
    char hash_tmp[32];
    while(1){
        if(sd->chain_height==MAX_BLOCKCHAIN_HEIGHT){
            printf("Max chain height reached, breaking out of loop\n");
            break;
        }
        
        // store current chain height to compare against
        // do mutex in case another process is writing and I read corrupted data
        sem_wait(chain_access_mutex);
        working_on_height=sd->chain_height;
        working_on_block=sd->chain[working_on_height];
        sem_post(chain_access_mutex);
        
        for(int i=0; i<2147483647; i++){
            if(sd->chain_height != working_on_height){
                // someone added a block
                // stop mining and work on new block
                printf("A new block has been added, abandoning current block\n");
                break;
            }
            
            working_on_block.nonce=i;
            if(is_good_block(&working_on_block, &target)){
                sem_wait(chain_access_mutex);
                if(sd->chain_height != working_on_height){
                    printf("Found a valid nonce, but someone beat me to it\n");
                }else{
                    printf("Found a valid nonce, storing to chain\n");
                    sd->chain[working_on_height]=working_on_block;
                    sd->chain_height+=1;
                    // generate a new block for everyone to mine
                    dsha(&working_on_block, sizeof(BitcoinHeader), &hash_tmp);
                    get_random_continuation_header(
                        &(sd->chain[sd->chain_height]),
                        &hash_tmp,
                        difficulty
                    );
                }
                sem_post(chain_access_mutex);
                break;
            }
        }
        
    }
    
    if(is_first_create){
        for(int i=0; i<MAX_BLOCKCHAIN_HEIGHT; i++){
            printf("Block %d:\n", i);
            debug_print_header(sd->chain[i]);
        }
        printf("The chain has been printed above.\n");
    }else{
        printf("Completed, exiting. The first instance should be printing the chain to the screen.\n");
    }
    
    
    close(fd);
    shm_unlink("/minechain");
    sem_unlink("/minechainsync");
    sem_unlink("/minechainmutex");
    return 0;
    
}



























