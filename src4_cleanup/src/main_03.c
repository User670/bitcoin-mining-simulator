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

#include <sha2.h>
#include <bitcoin_utils.h>
#include <data_utils.h>
#include <debug_utils.h>
#include <custom_errors.h>

#define NUM_PROCESSES 5
#define NUM_THREADS 5
#define SHARED_BUF_CHAIN_SIZE 1048576 //1MB
#define SHARED_BUF_BLOCK_SIZE 32768   //32KB



typedef struct{
    int result_found;
    char chain_data[SHARED_BUF_CHAIN_SIZE];
    char new_block_data[SHARED_BUF_BLOCK_SIZE];
    char* target;
    int no_more_jobs;
}SharedData1;

// global vars used by process thread miner get ptm_ prefix
BitcoinHeader ptm_header;
char* ptm_target;
int ptm_this_process_has_result;
int* ptm_some_process_has_result;
pthread_mutex_t ptm_flag_lock;
// just for printing purposes...
int ptm_process_id;

void *thread_miner(void *_id){
    BitcoinHeader block=ptm_header;
    int id=*(int*)_id;
    printf("THREAD %d-%d: Thread spawned\n", ptm_process_id, id);
    int start=2147483647/NUM_THREADS*id;
    int end= id==NUM_THREADS-1?2147483647:2147483647/NUM_THREADS*(id+1)-1;
    for(int i=start; i<=end; i++){
        if(ptm_this_process_has_result || *ptm_some_process_has_result){
            printf("THREAD %d-%d: someone found result, breaking\n", ptm_process_id, id);
            break;
        }
        block.nonce=i;
        if(is_good_block(&block, ptm_target)){
            pthread_mutex_lock(&ptm_flag_lock);
            // also actually make sure no one beat me to it
            if(ptm_this_process_has_result || *ptm_some_process_has_result){
                printf("THREAD %d-%d: I found a result, but someone beat me to it\n", ptm_process_id, id);
            }else{
                ptm_this_process_has_result=1;
                ptm_header.nonce=i;
                printf("THREAD %d-%d: Found valid nonce %d\n", ptm_process_id, id, i);
            }
            pthread_mutex_unlock(&ptm_flag_lock);
            break;
        }
        if(i==end){
            printf("end of loop\n");
        }
    }
    pthread_exit(NULL);
}

void process_miner(int id, SharedData1* sd){
    printf("CHILD %d: spawned\n",id);
    // prepare semaphore references
    sem_t* issue_job_sync_sem=sem_open("/issuejob", O_RDWR);
    sem_t* job_end_sync_sem=sem_open("/jobend", O_RDWR);
    sem_t* result_found_mutex=sem_open("/resultfound", O_RDWR);
    
    // prepare vars
    BitcoinBlock* new_block=NULL;
    BitcoinBlock* result_chain=NULL;
    
    int err;
    int rv; //return value
    
    ptm_process_id=id;
    
    // setup global signal ptr
    ptm_some_process_has_result=&(sd->result_found);
    
    // setup mutex
    pthread_mutex_init(&ptm_flag_lock, NULL);
    
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
        new_block=malloc(sizeof(BitcoinBlock));
        initialize_block(new_block, 0);
        rv=deserialize_block(new_block, sd->new_block_data, &err);
        if(rv==-1){
            perror_custom(err, NULL);
        }
        
        ptm_header=new_block->header;
        ptm_target=sd->target;
        ptm_this_process_has_result=0;
        
        // list of threads (I'm still unfamiliar lol)
        pthread_t threads[NUM_THREADS];
        int rc;
        int ids[NUM_THREADS];
        // spawn threads
        for(int i=0; i<NUM_THREADS; i++){
            // chatGPT
            ids[i]=i;
            rc = pthread_create(&threads[i], NULL, thread_miner, (void *)&(ids[i]));
            if (rc) {
                printf("Error creating thread %d\n", rc);
                exit(-1);
            }
        }
        
        // wait on threads
        // main thread doesn't seem to need to worry about killing other threads
        // they will suicide on signal
        for (int i=0; i<NUM_THREADS; i++) {
            // also chatGPT
            rc = pthread_join(threads[i], NULL);
            if (rc) {
                printf("Error joining thread %d\n", rc);
                exit(-1);
            }
        }
        
        // all threads suicided, which means one of two things:
        if(sd->result_found){
            printf("CHILD %d: Some other process has a result.\n", id);
            // one, some other process got a result
            // I guess nothing happens? just leave?
            // edit post BitcoinBlockv3: nope, free()
            recursive_free_block(new_block);
        }else{
            // two, this process got a result
            sem_wait(result_found_mutex);
            // *actually* make sure no one beat me to it
            if(sd->result_found){
                printf("CHILD %d: I found a nonce, but someone beat me to it in writing it to the shared memory.\n",id);
                recursive_free_block(new_block);
            }else{
                sd->result_found=1;
                new_block->header.nonce=ptm_header.nonce;
                // deserialize chain
                result_chain=malloc(sizeof(BitcoinBlock));
                initialize_block(result_chain,0);
                deserialize_blockchain(result_chain, sd->chain_data, NULL);
                // attach block
                attach_block(result_chain, new_block);
                // re-serialize block
                serialize_blockchain(result_chain, SHARED_BUF_CHAIN_SIZE, sd->chain_data, &err);
                
                recursive_free_blockchain(result_chain);
                printf("CHILD %d: found a valid nonce %d\n", id, ptm_header.nonce);
            }
            sem_post(result_found_mutex);
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
    
    
    // synchronization, or an attempt of it
    int num_tasks=10; // so that no endless loop is made
    
    // prepare shared memory
    int fd=shm_open("/minejobs", O_CREAT|O_RDWR, 0666);
    ftruncate(fd, sizeof(SharedData1));
    SharedData1* sd=mmap(NULL, sizeof(SharedData1), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    sd->result_found=0;
    sd->no_more_jobs=0;
    sd->target=target;
    
    // temporary vars
    BitcoinBlock* new_block=NULL;
    BitcoinBlock* chain;
    
    // create a dummy first block, so that children don't work with an empty chain
    chain=malloc(sizeof(BitcoinBlock));
    initialize_block(chain,0);
    get_dummy_genesis_block(chain);
    int err;
    int rv=serialize_blockchain(chain, SHARED_BUF_CHAIN_SIZE, sd->chain_data, &err);
    if(rv==-1){
        perror_custom(err, NULL);
    }
    
    // prepare semaphores
    sem_t* issue_job_sync_sem=sem_open("/issuejob",O_CREAT|O_RDWR, 0666, 0);
    sem_t* job_end_sync_sem=sem_open("/jobend",O_CREAT|O_RDWR, 0666, 0);
    /*sem_t* result_found_mutex=*/sem_open("/resultfound",O_CREAT|O_RDWR, 0666, 1);
    
    
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
        // on first loop, since new_block is NULL, this free does nothing
        // otherwise, it frees the block from previous loop
        recursive_free_block(new_block);
        new_block=malloc(sizeof(BitcoinBlock));
        initialize_block(new_block, difficulty);
        randomize_block(new_block);
        // the chain is either from before the loop, or end of previous loop
        obtain_last_block_hash(chain, new_block->header.previous_block_hash);
        
        serialize_block(new_block, SHARED_BUF_BLOCK_SIZE, sd->new_block_data, NULL);
        
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
        recursive_free_blockchain(chain);
        chain=malloc(sizeof(BitcoinBlock));
        initialize_block(chain, 0);
        deserialize_blockchain(chain, sd->chain_data, NULL);
        printf("Parent: the last attached block has nonce %d\n", obtain_last_nonce(chain));
    }
    sd->no_more_jobs=1;
    for(int fork_num=0; fork_num<NUM_PROCESSES; fork_num++){
        sem_post(issue_job_sync_sem);
    }
    
    for(int fork_num=0; fork_num<NUM_PROCESSES; fork_num++){
        wait(NULL);
    }
    
    shm_unlink("/issuejob");
    shm_unlink("/jobend");
    shm_unlink("/resultfound");
    
    return 0;
    
}



























