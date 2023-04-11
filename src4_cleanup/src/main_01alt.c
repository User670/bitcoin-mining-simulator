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
#include <sys/types.h>
#include <sys/wait.h>

#include <sha2.h>
#include <bitcoin_utils.h>
#include <data_utils.h>
#include <debug_utils.h>

#define NUM_PROCESSES 5

// for multiprocessing
// mines a single header
// @params
//  *block: pointer to a block to be mined
//  *target: pointer to a expanded target
// @return
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

void sig_hand(int sig){
    printf("%d> signal %d received -- terminating gracefully\n", getpid(), sig);
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
    
    BitcoinHeader block;
    get_random_header(&block, difficulty);
    
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
        
        printf("P> waiting for child completion\n");
        first_finisher=wait(NULL);
        
        printf("P> Signalling all other children\n");
        for (i = 0; i < NUM_PROCESSES; i++) {
            if (pid_child[i] != first_finisher)
                kill(pid_child[i],SIGUSR1);
        }

        printf("P> Checking all children have terminated\n");
        for (i = 0; i < NUM_PROCESSES; i++) {
            wait(0);
        }
        
    } else {
        setup_signal_handler(sigact, &set);
        
        mine_single_block(&block, target);
        
        printf("%d> found a result!\n", getpid());
        exit(0);
    }
    
    return 0;
    
}



























