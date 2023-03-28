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

#define NUM_PROCESSES 5
#define NUM_THREADS 5
#define MAX_BLOCKCHAIN_HEIGHT 10


// for multiprocessing
// mines a single header
// @params
//  *block: pointer to a block to be mined
//  *target: pointer to a expanded target
// @return
//  void
void mine_single_block(BitcoinHeader* block, const char* target){
    int i=0;
    int start=time(NULL);
    for(; i<21474836; i++){
        block->nonce=i;
        if(is_good_block(block, target)){
            //printf("CHILD PROCESS: nonce found %d\n", i);
            //break;
        }
        
    }
    int end=time(NULL);
    printf("Done, and it took %d seconds\n", end-start);
}

typedef struct{
    int result_found;
    BitcoinHeader block;
    char* target;
    int no_more_jobs;
}SharedData1;

// what do I need here? (for threads)
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
/*BitcoinHeader ptm_header;
char* ptm_target;
int ptm_this_process_has_result;
int* ptm_some_process_has_result;
pthread_mutex_t ptm_flag_lock;*/
// just for printing purposes...
int ptm_process_id;



typedef struct{
    int file_lock_counter;
    char* target;
}SharedData2;


// What do I need here...? (for files)
/*
Threads need...
- A counter from the shared memory, and two semaphores, to implement
  file lock
- A string, block file path
- still a header

The main thread of the child process needs...
- two strings, path to the two files
- the same counter and semaphores
- a max height macro
*/
// What to do with sync...?
/*
- threads read block count from file
- When it reads a new block, suicide
- When it mines a block, use ptm_ variable to signal other threads to suicide
- Process, when all threads suicide, check block count, write if no 
  block has been pushed by another process
*/

void *thread_miner(void *_id){
    BitcoinHeader block=ptm_header;
    int id=*(int*)_id;
    printf("THREAD %d-%d: Thread spawned\n", ptm_process_id, id);
    int start=2147483647/NUM_THREADS*id;
    int end= id==NUM_THREADS-1?2147483647:2147483647/NUM_THREADS*(id+1);
    for(int i=start; i<end; i++){
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
    
    BitcoinHeader block;
    get_random_header(&block, difficulty);
    mine_single_block(&block, &target);
    /*
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
    */
    
}



























