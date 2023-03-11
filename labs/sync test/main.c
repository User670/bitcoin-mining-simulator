/*
This was to test out the simplest way to inform other forks of someone
finding a nonce.

Results:
- Yes, one single boolean variable in the shared memory will do the trick,
  no semaphores required.
- Yes, this does create a problem of how I justify doing semaphores.
    - I think I'll make parent try to send forks more tasks, thus forks
      have to stay alive when they are done, and use semaphores to sleep
      until they are awaken by parent
- WSL (Windows Subsystem for Linux) Ubuntu and ACTUAL Ubuntu (on a VM)
  behave somewhat differently. Guess I still have to get files between
  my Windows machine and the VM back and forth a few times.
*/

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

// fake a few nonce goals, can't be bothered to do real hashes here
int targets[5]={10000000,10000000,10000000,10000000,20000};

void fork_func(int id, int* sig){
    printf("Fork %d spawned\n",id);
    for(int i=0; i<30000000; i++){
        if(*sig){
            printf("Fork %d: Someone reached their goal, breaking at i=%d\n",id, i);
            break;
        }
        if(i==targets[id]){
            printf("Fork %d: target reached\n", id);
            *sig=1;
            break;
        }
    }
}

int main(){
    int fd=shm_open("/lorem", O_CREAT|O_RDWR, 0666);
    ftruncate(fd, sizeof(int));
    int* s=mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    *s=0;
    pid_t pid;
    for(int i=0; i<5; i++){
        pid=fork();
        if(pid==0){
            fork_func(i, s);
            exit(0);
        }
    }
}