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

int main(){
    int pid=fork();
    sem_t* sem=sem_open("/crashchildtest",O_RDWR|O_CREAT, 0666, 0);
    
    if(pid==0){
        printf("child begin\n");
        char* a=malloc(100);
        free(a);
        free(a);
        sem_post(sem);
        printf("child end\n");
    }else{
        printf("parent begin\n");
        sem_wait(sem);
        wait(0);
        printf("parent end\n");
    }
}