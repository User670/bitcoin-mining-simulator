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
// params
//  header: header of the block to be mined
//  *target: pointer to a expanded target
// return
//  void
void mine_single_block(BitcoinHeader header, const char* target){
    int i=0;
    for(; i<2147483647; i++){
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
    sigact.sa_handler = sig_hand;
    sigact.sa_mask = *set;
    sigact.sa_flags = 0;
    sigaction(SIGUSR1,&sigact,0);
    sigdelset(set,SIGUSR1);
    sigprocmask(SIG_SETMASK, set, NULL);
}


int main(){
    // seed the random number generator
    srand(time(NULL));
    
    int difficulty=DIFFICULTY_1M;
    char target[32];
    construct_target(difficulty, &target);
    
    BitcoinBlock block;
    randomize_block_transactions(&block);
    
    // prep work for signal handler
    sigset_t set;
    struct sigaction sigact;
    sigfillset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);
    
    int i;
    pid_t pid_father = getpid();
    pid_t pid_child[NUM_PROCESSES];
    pid_t first_finisher;
    
    for (i = 0; (i<NUM_PROCESSES)&&((pid_child[i]=fork())!=0); i++)
        ;
    
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
        
        mine_single_block(block.header, target);
        
        printf("Child process with PID %d: found a result!\n", getpid());
        exit(0);
    }
    
    return 0;
    
}



























