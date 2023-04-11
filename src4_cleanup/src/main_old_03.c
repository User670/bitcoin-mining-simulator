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

#define CP(x) printf("Checkpoint %d\n",x);

#define NUM_PROCESSES 5
#define NUM_THREADS 5
#define SHARED_BUF_CHAIN_SIZE 1048576 //1MB
#define SHARED_BUF_BLOCK_SIZE 32768   //32KB


typedef struct{
    int result_found;
    BitcoinHeader block;
    char* target;
    int no_more_jobs;
}SharedData1;


// what do I need here?
/*
- the block itself
  - need to copy it so that it doesn't mess up with other threads
- a "this child has a result" signal
- a "some child has a result" signal
  - this one is a global shared memory location
- a numerical ID for this thread to determine where to loop
  - this is the only one unique to each thread in this process
- a mutex so that the flag write is synced
*/
// what to do with sync?
/*
- finding a nonce raises "this child has a result" flag
  - seeing this flag kicks all threads in this process
- parent can either wait on pthread_join() until all threads die, ~~or wait on a
  conditional to start a bit early~~
  (no, conditionals won't fire for another process's result)
- parent, after the wait, raises the "some child has a result" flag
  - threads suicide when seeing this flag, allowing parent to proceed
  - parent can see the flag to determine if it's their win or someone else's
*/

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

void process_thread_miner(int id, SharedData1* sd){
    printf("CHILD %d: spawned\n",id);
    // prepare semaphore references
    sem_t* issue_job_sync_sem=sem_open("/issuejob", O_RDWR);
    sem_t* job_end_sync_sem=sem_open("/jobend", O_RDWR);
    sem_t* result_found_mutex=sem_open("/resultfound", O_RDWR);
    
    // prepare vars
    //BitcoinHeader working_block;
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
        
        // setup job-specific var and reset flags
        ptm_header=sd->block;
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
        }else{
            // two, this process got a result
            sem_wait(result_found_mutex);
            // *actually* make sure no one beat me to it
            if(sd->result_found){
                printf("CHILD %d: I found a nonce, but someone beat me to it in writing it to the shared memory.\n",id);
            }else{
                sd->result_found=1;
                sd->block.nonce=ptm_header.nonce;
                printf("CHILD %d: found a valid nonce %d\n", id, ptm_header.nonce);
            }
            sem_post(result_found_mutex);
        }
        
        /*
        // old thread miner stuff here
        // make a copy of the block so that it doesn't mess with other processes
        working_block=sd->block;
        
        //printf("CHILD %d: I'm entering main loop\n",id);
        for(int i=0; i<=2147483647; i++){
            if(sd->result_found){
                // could have merged the conditional in the loop, but
                // 1, I don't like cramming conditions in the loop,
                // 2, I can print stuff here this way
                printf("CHILD %d: Someone found result; breaking at i=%d\n", id, i);
                break;
            }
            working_block.nonce=i;
            if(is_good_block(&working_block, sd->target)){
                //printf("CHILD %d: I found a nonce, waiting for mutex\n", id);
                sem_wait(result_found_mutex);
                //printf("CHILD %d: I'm in the mutex\n", id);
                // *actually* make sure no one beat me to it
                if(sd->result_found){
                    printf("CHILD %d: I found a nonce, but someone beat me to it in writing it to the shared memory.\n",id);
                }else{
                    sd->result_found=1;
                    sd->block.nonce=i;
                    printf("CHILD %d: found a valid nonce %d\n", id, i);
                }
                sem_post(result_found_mutex);
                //printf("CHILD %d: I'm out of the mutex\n", id);
                break;
            }
        }
        
        */
        // tell parent that I'm done
        sem_post(job_end_sync_sem);
    }
    
}


int main(){
    // seed the random number generator
    srand(time(NULL));
    
    // C warm up: brute force one block
    int difficulty=0x1f03a30c;
    char target[32];
    construct_target(difficulty, &target);
    
    /*BitcoinHeader block;
    block.version=2;
    get_random_hash(&(block.previous_block_hash));
    get_random_hash(&(block.merkle_root));
    block.timestamp=time(NULL);
    block.difficulty=difficulty;
    block.nonce=0;
    
    int i=0;
    for(; i<=2147483647; i++){
        block.nonce=i;
        if(is_good_block(&block, &target)){
            printf("nonce found: %d",i);
            break;
        }
    }*/
    
    // Multiprocessing
    /*
    int num_processes=5;
    BitcoinHeader blocks[num_processes]; // is this not kosher?
    pid_t pid;
    for(int fork_num=0; fork_num<num_processes; fork_num++){
        get_random_header(&blocks[fork_num], difficulty);
        pid=fork();
        if(pid==0){
            mine_single_block(&blocks[fork_num], &target);
            exit(0);
        }
    }
    
    for(int fork_num=0; fork_num<num_processes; fork_num++){
        wait(NULL);
    }
    */
    
    // synchronization, or an attempt of it
    int num_tasks=10; // so that no endless loop is made
    
    // prepare shared memory
    int fd=shm_open("/minejobs", O_CREAT|O_RDWR, 0666);
    ftruncate(fd, sizeof(SharedData1));
    SharedData1* sd=mmap(NULL, sizeof(SharedData1), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    sd->result_found=0;
    sd->no_more_jobs=0;
    sd->target=&target;
    
    //get_random_header(&(sd->block), &target);
    
    // prepare semaphores
    sem_t* issue_job_sync_sem=sem_open("/issuejob",O_CREAT|O_RDWR, 0666, 0);
    sem_t* job_end_sync_sem=sem_open("/jobend",O_CREAT|O_RDWR, 0666, 0);
    sem_t* result_found_mutex=sem_open("/resultfound",O_CREAT|O_RDWR, 0666, 1);
    
    
    pid_t pid;
    for(int fork_num=0; fork_num<NUM_PROCESSES; fork_num++){
        pid=fork();
        if(pid==0){
            process_thread_miner(fork_num, sd);
            exit(0);
        }
    }
    
    for(int task_num=0; task_num<num_tasks; task_num++){
        // prepare task
        get_random_header(&(sd->block), difficulty);
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
        printf("PARENT: A result was found with nonce=%d\n",sd->block.nonce);
    }
    
    sd->no_more_jobs=1;
    for(int fork_num=0; fork_num<NUM_PROCESSES; fork_num++){
        sem_post(issue_job_sync_sem);
    }
    
    for(int fork_num=0; fork_num<NUM_PROCESSES; fork_num++){
        wait(NULL);
    }
    
    return 0;
    
}



























