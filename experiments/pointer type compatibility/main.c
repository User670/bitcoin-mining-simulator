#include <stdio.h>
#include <stdlib.h>

typedef struct{
    int a;
    int b;
    char c[32];
    int d;
}t_typedef_struct;

void void_star(void* ptr){
    
}

void char_star(char* ptr){
    
}

void unsigned_char_star(unsigned char* ptr){
    
}

void char_array(char ptr[]){
    
}

void unsigned_char_array(unsigned char ptr[]){
    
}

void const_char_star(const char* ptr){
    
}

void const_unsigned_char_star(const unsigned char* ptr){
    
}

void typedef_struct(t_typedef_struct* ptr){
    
}

int main(){
    void* p_void_star=malloc(10);
    char* p_char_star=malloc(10);
    unsigned char* p_unsigned_char_star=malloc(10);
    char p_char_array[10];
    unsigned char p_unsigned_char_array[10];
    const char* p_const_char_star=malloc(10);
    const unsigned char* p_const_unsigned_char_star=malloc(10);
    char p_char_array_eq_literal[]="literal";
    char* p_char_star_eq_literal="literal";
    t_typedef_struct* p_typedef_struct=malloc(sizeof(t_typedef_struct));
    
    
    /*
    void_star(p_void_star);
    void_star(p_char_star);
    void_star(p_unsigned_char_star);
    void_star(p_char_array);
    void_star(p_unsigned_char_array);
    void_star(p_const_char_star); //discard const
    void_star(p_const_unsigned_char_star); //discard const
    void_star(p_char_array_eq_literal);
    void_star(p_char_star_eq_literal);
    void_star("literal_string");

    char_star(p_void_star);
    char_star(p_char_star);
    char_star(p_unsigned_char_star); //different signedness
    char_star(p_char_array);
    char_star(p_unsigned_char_array); //different signedness
    char_star(p_const_char_star); //discard const
    char_star(p_const_unsigned_char_star); //discard const
    char_star(p_char_array_eq_literal);
    char_star(p_char_star_eq_literal);
    char_star("literal_string");

    unsigned_char_star(p_void_star);
    unsigned_char_star(p_char_star); //different signedness
    unsigned_char_star(p_unsigned_char_star);
    unsigned_char_star(p_char_array); //different signedness
    unsigned_char_star(p_unsigned_char_array);
    unsigned_char_star(p_const_char_star); //discard const
    unsigned_char_star(p_const_unsigned_char_star); //discard const
    unsigned_char_star(p_char_array_eq_literal); //different signedness
    unsigned_char_star(p_char_star_eq_literal); //different signedness
    unsigned_char_star("literal_string"); //different signedness

    char_array(p_void_star);
    char_array(p_char_star);
    char_array(p_unsigned_char_star); //different signedness
    char_array(p_char_array);
    char_array(p_unsigned_char_array); //different signedness
    char_array(p_const_char_star); //discard const
    char_array(p_const_unsigned_char_star); //discard const
    char_array(p_char_array_eq_literal);
    char_array(p_char_star_eq_literal);
    char_array("literal_string");

    unsigned_char_array(p_void_star);
    unsigned_char_array(p_char_star);
    unsigned_char_array(p_unsigned_char_star);
    unsigned_char_array(p_char_array);
    unsigned_char_array(p_unsigned_char_array);
    unsigned_char_array(p_const_char_star);
    unsigned_char_array(p_const_unsigned_char_star);
    unsigned_char_array(p_char_array_eq_literal);
    unsigned_char_array(p_char_star_eq_literal);
    unsigned_char_array("literal_string");

    const_char_star(p_void_star);
    const_char_star(p_char_star);
    const_char_star(p_unsigned_char_star);
    const_char_star(p_char_array);
    const_char_star(p_unsigned_char_array);
    const_char_star(p_const_char_star);
    const_char_star(p_const_unsigned_char_star);
    const_char_star(p_char_array_eq_literal);
    const_char_star(p_char_star_eq_literal);
    const_char_star("literal_string");

    const_unsigned_char_star(p_void_star);
    const_unsigned_char_star(p_char_star);
    const_unsigned_char_star(p_unsigned_char_star);
    const_unsigned_char_star(p_char_array);
    const_unsigned_char_star(p_unsigned_char_array);
    const_unsigned_char_star(p_const_char_star);
    const_unsigned_char_star(p_const_unsigned_char_star);
    const_unsigned_char_star(p_char_array_eq_literal);
    const_unsigned_char_star(p_char_star_eq_literal);
    const_unsigned_char_star("literal_string");
    */
    /*
    general observations:
    - literal strings are char*
    - arrays (defined with []) are still *
    - void* takes anything so far, except when passed-in arg is const
    - signedness is a common warning
    - const takes both const and non-const; discarding const is a common warning
    */
    /*
    void_star(p_typedef_struct);

    char_star(p_typedef_struct);

    unsigned_char_star(p_typedef_struct);

    char_array(p_typedef_struct);

    unsigned_char_array(p_typedef_struct);

    const_char_star(p_typedef_struct);

    const_unsigned_char_star(p_typedef_struct);

    typedef_struct(p_void_star);
    typedef_struct(p_char_star);
    typedef_struct(p_unsigned_char_star);
    typedef_struct(p_char_array);
    typedef_struct(p_unsigned_char_array);
    typedef_struct(p_const_char_star);
    typedef_struct(p_const_unsigned_char_star);
    typedef_struct(p_char_array_eq_literal);
    typedef_struct(p_char_star_eq_literal);
    typedef_struct("literal_string");
    typedef_struct(p_typedef_struct);
    */
    /*
    observations:
    - void can take anything
    - void can go into anything
    - anything to do with char is incompatible with custom struct types
      (both ways)
    */
    
    // I still don't know what causes the compiler to think I have a 
    // "char (*)[32] type value, which is incompatible with char*
    
    typedef_struct(&p_char_array);
    typedef_struct(&(p_typedef_struct->c));
}