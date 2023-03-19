#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#define NB_CHILDREN 4

int compute(){
    int i, max;
    srand(getpid());
    max = rand()%8*10000000;
    for (i = 0; i < max; i++);
    return max;
}


int count = 0;
void sig_hand(int sig){
    printf("%d> signal %d received -- terminating gracefully\n", getpid(), sig);
    exit(0);
}

int main(){
    // a set of signals
    sigset_t set;
    // ??
    struct sigaction sigact;
    
    // fill the set with all signals
    sigfillset(&set);
    // ??
    sigprocmask(SIG_SETMASK, &set, NULL);
    
    int i;
    pid_t pid_father = getpid();
    pid_t pid_child[NB_CHILDREN];
    pid_t first_finisher;

    for (i = 0; (i<NB_CHILDREN)&&((pid_child[i]=fork())!=0); i++)
        ;
    
    if (getpid() == pid_father){
        int return_value=0;
        
        printf("P> waiting for child completion\n");
        first_finisher = wait(&return_value);
        
        printf("Got return value %d\n", WEXITSTATUS(return_value));
        
        printf("P> Signalling all other children\n");
        for (i = 0; i < NB_CHILDREN; i++) {
            if (pid_child[i] != first_finisher)
                kill(pid_child[i],SIGUSR1);
        }

        printf("P> Checking all children have terminated\n");
        for (i = 0; i < NB_CHILDREN; i++) {
            wait(0);
        }
        
    } else {
        sigact.sa_handler = sig_hand;
        sigact.sa_mask = set;
        sigact.sa_flags = 0;
        // change action on signal 
        sigaction(SIGUSR1,&sigact,0);
        //                        ^
        //                   ... shouldn't this be *oldact
        //                   why does 0 work, is it NULL?
        
        
        sigdelset(&set,SIGUSR1);
        sigprocmask(SIG_SETMASK, &set, NULL);
        
        //int v=compute();
        compute();
        
        printf("%d> found a result!\n", getpid());
        exit(256);
    }
    
    printf("End of program for %d\n",i);
    return EXIT_SUCCESS;    
    
}
