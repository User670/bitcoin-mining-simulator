#include <stdio.h>

// Print a continuous part of memory in hexadecimal.
// params
//  *data: pointer to the start of the memory to print
//  bytes: how many bytes to print
// return
//  void
void debug_print_hex(void* data, int bytes){
    for(int i=0; i<bytes; i++){
        printf("%02X",*(unsigned char*)(data+i));
    }
}

// Same as debug_print_hex, but also prints a \n at the end.
// params
//  *data: pointer to the start of the memory to print
//  bytes: how many bytes to print
// return
//  void
void debug_print_hex_line(void* data, int bytes){
    debug_print_hex(data, bytes);
    printf("\n");
}