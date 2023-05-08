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
#include <errno.h>

#include <sha2.h>
#include <bitcoin_utils.h>
#include <data_utils.h>
#include <debug_utils.h>
#include <custom_errors.h>

#define NUM_PROCESSES 5
#define MAX_BLOCKCHAIN_HEIGHT 10


typedef struct{
    // TODO: add more fields if you need
    int chain_height;
    BitcoinBlock new_block;
    char genesis_block_name[100];
}SharedData;


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
    
    int difficulty=DIFFICULTY_1M;
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
    
    int rv;
    
    // prepare shared memory
    int fd;
    int is_first_create=0;
    fd=shm_open("/minechain", O_CREAT|O_EXCL|O_RDWR, 0666);
    if(fd==-1){
        if(errno!=EEXIST){
            perror("first shm_open for /minechain");
            exit(1);
        }
        fd=shm_open("/minechain", O_CREAT|O_RDWR, 0666);
        if(fd==-1){
            perror("second shm_open for /minechain");
            exit(1);
        }
    }else{
        is_first_create=1;
    }
    rv=ftruncate(fd, sizeof(SharedData));
    if(rv==-1){
        perror("ftruncate for /minechain");
        exit(1);
    }
    SharedData* sd=mmap(NULL, sizeof(SharedData), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(sd==MAP_FAILED){
        perror("mmap for /minechain");
        exit(1);
    }
    
    int process_count;
    sem_t* start_sync_sem=sem_open("/minechainsync", O_CREAT|O_RDWR, 0666, 0);
    
    if(is_first_create){
        // initialize the shared data
        sd->chain_height=0;
        
        BitcoinBlock* _genesis=malloc(sizeof(BitcoinBlock));
        get_dummy_genesis_block(_genesis);
        char _hash[32];
        char _name[100];
        dsha(&(_genesis->header), sizeof(BitcoinHeader), _hash);
        construct_shm_name(_hash, _name);
        rv=write_block_in_shm(_name, _genesis);
        if(rv<0){
            perror_custom(rv, "write_block_in_shm for genesis");
            exit(1);
        }
        strcpy(sd->genesis_block_name, _name);
        
        BitcoinBlock* _new_block=malloc(sizeof(BitcoinBlock));
        initialize_block(_new_block, difficulty);
        memcpy(&(_new_block->header.previous_block_hash), _hash, 32);
        randomize_block_transactions(_new_block);
        memcpy(&(sd->new_block), _new_block, sizeof(BitcoinBlock));
        
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
    
    // TODO: initialize more mutex if needed
    sem_t* chain_access_mutex=sem_open("/minechainmutex", O_CREAT|O_RDWR, 0666, 1);
    
    int working_on_height;
    BitcoinHeader working_on_header;
    BitcoinBlock* new_block;
    while(1){
        // TODO: a lot of logic needs to be added here!
        if(sd->chain_height==MAX_BLOCKCHAIN_HEIGHT){
            printf("Max chain height reached, breaking out of loop\n");
            break;
        }
        
        // do mutex in case another process is writing and I read corrupted data
        sem_wait(chain_access_mutex);
        
        new_block=malloc(sizeof(BitcoinBlock));
        memcpy(new_block, &(sd->new_block), sizeof(BitcoinBlock));
        working_on_header=new_block->header;
        
        sem_post(chain_access_mutex);
        
        // Again, this is a basic loop, and you need to insert more logic
        for(int i=0; i<2147483647; i++){
            
            working_on_header.nonce=i;
            if(is_good_block(&working_on_header, target)){
                
                printf("Found a valid nonce, storing to chain\n");
                
                // attach the block to the blockchain
                new_block->header.nonce=i;
                char hash[32];
                dsha(&(new_block->header), sizeof(BitcoinBlock), hash);
                char name[100];
                rv=attach_block(sd->genesis_block_name, new_block, name);
                if(rv<0){
                    perror_custom(rv, "main loop -> attach_block");
                }
                write_block_in_shm(name, new_block);
                
                sd->chain_height+=1;
                // generate a new block for everyone to mine
                initialize_block(new_block, difficulty);
                memcpy(&(new_block->header.previous_block_hash), hash, 32);
                randomize_block_transactions(new_block);
                memcpy(&(sd->new_block), new_block, sizeof(BitcoinBlock));
                    
                break;
            }
        }
        
    }
    
    
    // Finishing up
    if(is_first_create){
        char name1[100];
        char name2[100];
        strcpy(name1,sd->genesis_block_name);
        BitcoinBlock* block=malloc(sizeof(BitcoinBlock));
        while(1){
            rv=get_block_info(name1, name2, NULL, block);
            if(rv<0){
                perror_custom(rv,"get_block_info");
                break;
            }
            printf("Header of block %s:\n",name1);
            debug_print_header(block->header);
            if(name2[0]==0)break;
            strncpy(name1, name2, 100);
        }
        printf("The headers in the chain has been printed above.\n");
    }else{
        printf("Completed, exiting. The first instance should be printing the chain to the screen.\n");
    }
    
    
    close(fd);
    shm_unlink("/minechain");
    sem_unlink("/minechainsync");
    sem_unlink("/minechainmutex");
    return 0;
    
}



























