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

// for multiprocessing
// mines a single header
// @params
//  header: header of the block to be mined
//  *target: pointer to a expanded target
// @return
//  void
void mine_single_block(BitcoinHeader header, const char* target){
    int i=0;
    for(; i<=2147483647; i++){
        header.nonce=i;
        if(is_good_block(&header, target)){
            printf("mine_single_block: nonce found %d\n", i);
            break;
        }
    }
}

void sig_hand(int sig){
    printf("Child with PID %d: signal %d received - terminating gracefully\n", getpid(), sig);
    exit(0);
}

void setup_signal_handler(struct sigaction sigact, sigset_t* set){
    // set the `sigact` var with... an action? didn't find docs yet
    sigact.sa_handler = sig_hand;
    sigact.sa_mask = *set;
    sigact.sa_flags = 0;
    // change action on signal SIGUSR1 
    sigaction(SIGUSR1,&sigact,0);
    //                        ^
    //                   ... shouldn't this be *oldact
    //                   why does 0 work, is it NULL?
    
    // un-block SIGUSR1
    sigdelset(set,SIGUSR1);
    sigprocmask(SIG_SETMASK, set, NULL);
}


int main(){
    // seed the random number generator
    srand(time(NULL));
    
    // C warm up: brute force one block
    int difficulty=DIFFICULTY_1M;
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
    /*int num_processes=5;
    //BitcoinHeader blocks[num_processes]; // is this not kosher?
    BitcoinHeader block;
    get_random_header(&block, difficulty);
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
    
    // Multiprocessing with kills
    
    BitcoinBlock block;
    randomize_block_transactions(&block);
    
    // from an example code from my professor (no names for privacy)
    // a set (in mathematical sense) of signals
    sigset_t set;
    // "what to do on actions" for later
    struct sigaction sigact;
    
    // set the `set` to all signals
    sigfillset(&set);
    // block all signals, ... other than SIGSTOP and SIGKILL which can't be blocked
    sigprocmask(SIG_SETMASK, &set, NULL);
    // Perplexity AI (ChatGPT) says you shouldn't block signals for a long time...
    
    int i;
    pid_t pid_father = getpid();
    pid_t pid_child[NUM_PROCESSES];
    pid_t first_finisher;
    
    for (i = 0; (i<NUM_PROCESSES)&&((pid_child[i]=fork())!=0); i++)
        ;
    // that's confusing for me tbh
    // I'd write:
    // for(i=0; i<num_processes; i++){
    //     pid_child[i]=fork();
    //     if(pid_child[i]==0) break;
    // }
    // does the code run faster or something if the fork is in the conditions?
    
    if (getpid() == pid_father){
        
        printf("Parent: waiting for child completion\n");
        first_finisher=wait(NULL);
        
        
        printf("Parent: Signalling all other children\n");
        for (i = 0; i < NUM_PROCESSES; i++) {
            if (pid_child[i] != first_finisher)
                kill(pid_child[i],SIGUSR1);
        }

        printf("Parent: Checking all children have terminated\n");
        for (i = 0; i < NUM_PROCESSES; i++) {
            wait(0);
        }
        
    } else {
        setup_signal_handler(sigact, &set);
        
        //int v=compute();
        //compute();
        mine_single_block(block.header, target);
        
        printf("Child process with PID %d: found a result!\n", getpid());
        exit(0);
    }
    
    return 0;
    
}



























