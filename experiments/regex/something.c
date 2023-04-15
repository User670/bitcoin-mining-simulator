#include "something.h"
#include <regex.h>
#include <stdio.h>

#ifndef LOREM
#define LOREM

//regex_t some_regex;

void build_regex(){
    printf("build_regex()\n");
    int r=regcomp(&some_regex, "^/btcblock-[0-9a-f]{64}$", REG_NOSUB);
    if(r!=0){
        printf("Error on regcomp: %d\n", r);
    }
}

//build_regex();

#endif