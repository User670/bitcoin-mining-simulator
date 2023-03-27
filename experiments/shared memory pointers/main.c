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
#include <errno.h>

int main(){
    
    int fd;
    
    fd=shm_open("/minejobs", O_CREAT|O_RDWR, 0666);
    ftruncate(fd, sizeof(int*));
    int** sd=mmap(NULL, sizeof(int*), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    
    int a=8172438;
    int* p=&a;
    *sd=p;
    
    sem_t* sem=sem_open("/blah1", O_CREAT|O_RDWR, 0666, 0);
    
    pid_t pid=fork();
    if(pid==0){
        sem_wait(sem);
        printf("%d\n", **sd);
    }else{
        a=18624521;
        sem_post(sem);
    }
    sem_unlink("/blah1");
}