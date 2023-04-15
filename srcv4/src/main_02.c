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

#define NUM_PROCESSES 5



typedef struct{
    int result_found;
    BitcoinBlock new_block;
    char genesis_block_name[100];
    char* target;
    int no_more_jobs;
}SharedData1;

void process_miner(int id, SharedData1* sd){
    printf("CHILD %d: spawned\n",id);
    // prepare semaphore references
    sem_t* issue_job_sync_sem=sem_open("/issuejob", O_RDWR);
    sem_t* job_end_sync_sem=sem_open("/jobend", O_RDWR);
    sem_t* result_found_mutex=sem_open("/resultfound", O_RDWR);
    
    // prepare vars
    BitcoinBlock working_block;
    BitcoinHeader working_header;
    char working_block_shm_name[100];

    int rv; //return value
    // main loop
    while(1){
        // wait for parent to issue job and release lock
        //printf("CHILD %d: I'm waiting for parent to release issuejob\n",id);
        sem_wait(issue_job_sync_sem);
        //printf("CHILD %d: I'm released from issuejob\n",id);
        // check `sd` for no-more-jobs signal
        if(sd->no_more_jobs){
            printf("CHILD %d: No more job flag raised, breaking out of main loop\n", id);
            break;
        }
        
        //deserialize the data in shared mem
        working_block=sd->new_block;
        working_header=working_block.header;
        
        //printf("CHILD %d: I'm entering main loop\n",id);
        for(int i=0; i<2147483647; i++){
            if(sd->result_found){
                // could have merged the conditional in the loop, but
                // 1, I don't like cramming conditions in the loop,
                // 2, I can print stuff here this way
                printf("CHILD %d: Someone found result; breaking at i=%d\n", id, i);
                break;
            }
            working_header.nonce=i;
            if(is_good_block(&working_header, sd->target)){
                sem_wait(result_found_mutex);
                // *actually* make sure no one beat me to it
                if(sd->result_found){
                    printf("CHILD %d: I found a nonce, but someone beat me to it in writing it to the shared memory.\n",id);
                }else{
                    sd->result_found=1;
                    working_block.header.nonce=i;
                    // okay v4 workflow
                    // first we do attach
                    rv=attach_block(sd->genesis_block_name, &working_block, working_block_shm_name);
                    if(rv<0){
                        perror_custom(rv, "child process -> attach block");
                    }
                    // and we write the block to shm
                    rv=write_block_in_shm(working_block_shm_name, &working_block);
                    if(rv<0){
                        perror_custom(rv, "child process -> write_block_in_shm");
                    }
                    
                    printf("CHILD %d: found a valid nonce %d\n", id, i);
                }
                sem_post(result_found_mutex);
                //printf("CHILD %d: I'm out of the mutex\n", id);
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
    
    // C warm up: brute force one block
    int difficulty=DIFFICULTY_1M;
    char target[32];
    construct_target(difficulty, &target);
    
    int rv; // for receiving return values of certain functions to catch errors
    
    // synchronization, or an attempt of it
    int num_tasks=10; // so that no endless loop is made
    
    // prepare shared memory
    int fd=shm_open("/minejobs", O_CREAT|O_RDWR, 0666);
    if(fd==-1){
        perror("main -> shm_open for jobs");
        exit(1);
    }
    rv=ftruncate(fd, sizeof(SharedData1));
    if(rv==-1){
        perror("main -> ftruncate for jobs");
        exit(1);
    }
    SharedData1* sd=mmap(NULL, sizeof(SharedData1), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(sd==MAP_FAILED){
        perror("main -> mmap for jobs");
        exit(1);
    }
    sd->result_found=0;
    sd->no_more_jobs=0;
    sd->target=target;
    
    
    
    
    BitcoinBlock* genesis_block=malloc(sizeof(BitcoinBlock));
    get_dummy_genesis_block(genesis_block);
    
    char genesis_hash[32];
    dsha(&(genesis_block->header), sizeof(BitcoinHeader), genesis_hash);
    char genesis_block_name[100];
    construct_shm_name(genesis_hash, genesis_block_name);
    
    rv=write_block_in_shm(genesis_block_name, genesis_block);
    if(rv<0){
        perror_custom(rv, "main -> write_block_in_shm for genesis");
        exit(1);
    }
    
    strcpy(sd->genesis_block_name, genesis_block_name);
    
    
    
    // prepare semaphores
    sem_t* issue_job_sync_sem=sem_open("/issuejob",O_CREAT|O_RDWR, 0666, 0);
    if(issue_job_sync_sem==SEM_FAILED){
        perror("main -> sem_open for /issuejob");
        exit(1);
    }
    sem_t* job_end_sync_sem=sem_open("/jobend",O_CREAT|O_RDWR, 0666, 0);
    if(job_end_sync_sem==SEM_FAILED){
        perror("main -> sem_open for /jobend");
        exit(1);
    }
    sem_t* result_found_mutex=sem_open("/resultfound",O_CREAT|O_RDWR, 0666, 1);
    if(result_found_mutex==SEM_FAILED){
        perror("main -> sem_open for /resultfound");
        exit(1);
    }
    
    BitcoinBlock* new_block=malloc(sizeof(BitcoinBlock));
    
    pid_t pid;
    for(int fork_num=0; fork_num<NUM_PROCESSES; fork_num++){
        pid=fork();
        if(pid==0){
            process_miner(fork_num, sd);
            exit(0);
        }
    }
    
    for(int task_num=0; task_num<num_tasks; task_num++){
        // prepare task
        initialize_block(new_block, difficulty);
        randomize_block_transactions(new_block);
        // the chain is either from before the loop, or end of previous loop
        rv=get_last_block_hash(genesis_block_name, new_block->header.previous_block_hash);
        if(rv<0){
            perror_custom(rv, "main, main loop -> get_last_block_hash");
        }
        
        memcpy(&(sd->new_block), new_block, sizeof(BitcoinBlock));
        
        sd->result_found=0;
        
        // release children
        for(int fork_num=0; fork_num<NUM_PROCESSES; fork_num++){
            sem_post(issue_job_sync_sem);
        }
        
        // wait for children to finish
        for(int fork_num=0; fork_num<NUM_PROCESSES; fork_num++){
            // could have merged with previous loop but, eh, it's clearer
            // this way
            sem_wait(job_end_sync_sem);
        }
        
        // we should now have a result we could print
        // ... except we need to deserialize the chain to obtain said value
        rv=get_last_block_data(genesis_block_name, new_block);
        if(rv<0){
            perror_custom(rv, "main, main loop -> get_last_block_data");
        }
        printf("Parent: the last attached block has nonce %d\n", new_block->header.nonce);
    }
    sd->no_more_jobs=1;
    for(int fork_num=0; fork_num<NUM_PROCESSES; fork_num++){
        sem_post(issue_job_sync_sem);
    }
    
    for(int fork_num=0; fork_num<NUM_PROCESSES; fork_num++){
        wait(NULL);
    }
    
    sem_unlink("/issuejob");
    sem_unlink("/jobend");
    sem_unlink("/resultfound");
    
    close(fd);
    shm_unlink("minejobs");
    munmap(sd, sizeof(SharedData1));
    
    char fail[100];
    rv=unlink_shared_memories(genesis_block_name, fail);
    if(rv<0){
        perror_custom(rv, "main -> unlink_shared_memories");
        printf("The first shared memory to have not successfullt been unlinked is %s\n", fail);
    }
    
    return 0;
    
}



























