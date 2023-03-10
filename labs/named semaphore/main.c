/*
The purpose of this test is to see how named semaphores work.
Looks like I can just open semaphores by name in forks, without
having to pass around pointers in shared memory.
*/

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

int fork_func(){
    sem_t* a;
    while(1){
        a=sem_open("/ipsum", O_RDWR);
        if(a!=SEM_FAILED)break;
    }
    int b;
    sem_getvalue(a, &b);
    printf("FORK: sem value is %d\n",b);
    exit(0);
}

int main(){
    int pid=fork();
    if(pid==0){
        fork_func();
    }else{
        //parent code goes here
        sem_t* lorem=sem_open("/ipsum", O_CREAT|O_RDWR, 0666, 123);
        wait();
    }
}