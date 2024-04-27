/**
 * This file contains all source code for general purpose functions. 
 * These functions are used in both the client program and the orquestrator program.
*/
#include <stdlib.h>

/**
 * Translates integer value into string, the number being translated in the given base(radix).
 * value - integer value to translate
 * buffer - buffer to hold information
 * radix - base of conversion
 * returns length of the number string
*/
int itoa(int value, char *buffer, int radix) {
    if(radix < 2) return -1;

    char tmp[32];
    char *tp;

    int i;
    unsigned v;

    int sign = (radix == 10 && value < 0);
    if(sign) v = -value;
    else v = (unsigned)value;

    while(v || tp == tmp) {
        i = v % radix;
        v/= radix;
        if(i < 10) *tp++ = i+'0';
        else *tp++ = i + 'a' -10;
    } 

    int len = tp - tmp;

    if(sign) {
        *buffer++ = '-';
        len++;
    }

    while(tp > tmp) 
        *buffer++ = *--tp;
    
    *buffer = '\0';

    return len;
}

/**
 * Checks if given string is formatted as valid (positive) integer.
 * returns 0 - not valid
 * returns 1 - all good
*/
int is_positive_integer (const char* str) { 
    size_t i;
    size_t len = strlen(str);
    for(i = 0; i < len; i++) {
        if(str[i] < '0' || str[i] > '9') return 0;
    }
    return 1;
}

