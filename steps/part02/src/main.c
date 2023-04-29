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
    BitcoinBlock new_block;
    char genesis_block_name[100];
    char* target;
    // TODO: add more fields that you need to make this work!
}SharedData;

void process_miner(int id, SharedData* sd){
    printf("CHILD %d: spawned\n",id);
    // TODO: prepare semaphore references
    
    
    // prepare vars
    BitcoinBlock working_block;
    BitcoinHeader working_header;
    char working_block_shm_name[100];

    int rv; //return value
    // main loop
    while(1){
        // TODO
        
        // make a copy of the data to work on, so that it doesn't mess up other
        // processes. Make sure the parent has prepared a new task before this
        // point!
        working_block=sd->new_block;
        working_header=working_block.header;
        
        // Here is a basic brute force loop, but you need to modify it to make it
        // work properly.
        for(int i=0; i<2147483647; i++){
            
            working_header.nonce=i;
            if(is_good_block(&working_header, sd->target)){
                
                // This chunk of code writes the block to its corresponding
                // shared memory, while also updating the next/previous_block
                // references.
                working_block.header.nonce=i;
                // attach
                rv=attach_block(sd->genesis_block_name, &working_block, working_block_shm_name);
                if(rv<0){
                    perror_custom(rv, "child process -> attach block");
                }
                // write the block to shm
                rv=write_block_in_shm(working_block_shm_name, &working_block);
                if(rv<0){
                    perror_custom(rv, "child process -> write_block_in_shm");
                }
                printf("CHILD %d: found a valid nonce %d\n", id, i);
                // Write-to-shared-memory code ends here.
                
                break;
            }
        }
        
        
    }
}


int main(){
    // seed the random number generator
    srand(time(NULL));
    
    int difficulty=DIFFICULTY_1M;
    char target[32];
    construct_target(difficulty, &target);
    
    int rv; // for receiving return values of certain functions to catch errors
    
    int num_tasks=10; // so that no endless loop is made
    
    SharedData* sd;
    // TODO: prepare shared memory.
    // Open a shared memory to hold the SharedData, and map it to the pointer `sd`.
    
    
    sd->target=target;
    
    // TODO: prepare semaphores
    
    
    
    // This part creates a dummy genesis block, and writes it to its shared
    // memory. This is to simplify the children's job, as it avoids having to
    // deal with an empty blockchain for the children.
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
    // Dummy genesis creation ends here.
    
    
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
        rv=get_last_block_hash(genesis_block_name, new_block->header.previous_block_hash);
        if(rv<0){
            perror_custom(rv, "main, main loop -> get_last_block_hash");
        }
        memcpy(&(sd->new_block), new_block, sizeof(BitcoinBlock));
        
        // TODO: synchronization
        
        
        // we should now have a result we could print
        rv=get_last_block_data(genesis_block_name, new_block);
        if(rv<0){
            perror_custom(rv, "main, main loop -> get_last_block_data");
        }
        printf("Parent: the last attached block has nonce %d\n", new_block->header.nonce);
    }
    // TODO: Can we now signal the children to stop?
    
    
    for(int fork_num=0; fork_num<NUM_PROCESSES; fork_num++){
        wait(NULL);
    }
    
    // Cleanup
    // TODO: Unlink semaphores
    
    
    // TODO: clean up the shared memory for SharedData.
    // (There is a shared memory, a file descriptor and a mapped pointer
    //  to clean up.)
    
    
    // free pointers that have been malloc'd
    free(genesis_block);
    free(new_block);
    // This cleans up the shared memory used by the blockchain.
    char fail[100];
    rv=unlink_shared_memories(genesis_block_name, fail);
    if(rv<0){
        perror_custom(rv, "main -> unlink_shared_memories");
        printf("The first shared memory to have not successfully been unlinked is %s\n", fail);
    }
    
    return 0;
    
}



























