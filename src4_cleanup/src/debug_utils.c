#include <stdio.h>
#include <time.h>
#include <bitcoin_utils.h>

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

// Prints a header in a (mostly) human-readable way.
// params
//  block: the header.
// return
//  void
void debug_print_header(BitcoinHeader block){
    time_t timestamp = block.timestamp;
    struct tm *timeinfo = localtime(&timestamp);
    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S (%Z)", timeinfo);
    
    printf("====Debug print header start====\n");
    printf("version:       %d\n", block.version);
    printf("prev blk hash: ");
      debug_print_hex(&(block.previous_block_hash),32);
      printf("\n");
    printf("merkle root:   ");
      debug_print_hex(&(block.merkle_root),32);
      printf("\n");
    printf("timestamp:     %d (%s)\n", block.timestamp, buffer);
    printf("difficulty:    %X\n", block.difficulty);
    printf("nonce:         %d\n", block.nonce);
    printf("====Debug print header end====\n");
    
}