/*
Only if I'd asked ChatGPT sooner.

void change_value(int x){
    x=10;
}

int main(){
    int x=5;
    change_value(x);
    // here x is still 5
}

Have to pass in pointers to get anything modified by a function.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void debug_print_hex(void* data, int bytes){
    for(int i=0; i<bytes; i++){
        printf("%02X",*(unsigned char*)(data+i));
    }
}

void debug_print_hex_line(void* data, int bytes){
    for(int i=0; i<bytes; i++){
        printf("%02X",*(unsigned char*)(data+i));
    }
    printf("\n");
}

typedef struct{
    int a;
    char b[32];
} header;

typedef struct{
    header header;
} block;

void block_memory_copyer(block b){
    printf("ptr %d\n",&b);
    char s[32]="foo bar baz what comes next";
    memcpy(&(b.header.a),&s,32);
}

void block_ptr_memory_copyer(block* b){
    printf("ptr %d\n",b);
    char s[32]="foo bar baz this is pointer";
    memcpy(&(b->header.a),&s,32);
}

int main(){
    char s[32]="Lorem ipsum dolor sit amet";
    
    block a;
    printf("ptr %d\n",&a);
    a.header.a=42;
    printf("%d\n",a.header.a);
    memcpy(&(a.header.b),&s,32);
    debug_print_hex_line(&(a.header.b), 32);
    
    block_memory_copyer(a); // this fails to change the value
    
    debug_print_hex_line(&(a.header.b), 32);
    block_ptr_memory_copyer(&a); // but this does
    debug_print_hex_line(&(a.header.b), 32);
    
    block* b=malloc(sizeof(block)); // segfault if there is no "malloc"
    b->header.a=42;
    printf("%d\n",b->header.a);
    memcpy(&(b->header.b),&s,32);
    debug_print_hex_line(&(b->header.b), 32);
    
    free(b);
    
}