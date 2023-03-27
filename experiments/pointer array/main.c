#include <stdio.h>

int main(){
    int* a=malloc(10*sizeof(int));
    
    for(int i=0; i<10; i++){
        *(a+i)=i;
    }
    
    printf("1: %d\n", a+5);
    printf("2: %d\n", &a[5]);
    
}