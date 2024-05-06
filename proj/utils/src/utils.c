/**
 * This file contains all source code for general purpose functions. 
 * These functions are used in both the client program and the orquestrator program.
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"

/**
 * Translates integer value into string, the number being translated in the given base(radix).
 * value - integer value to translate
 * buffer - buffer to hold information
 * radix - base of conversion
 * returns length of the number string
*/
int itoa (int value, char *buffer, int radix) {
    char temp[33];
    int i = 0, sign = 0;

    if(value == 0) {
        buffer[0] = '0';
        buffer[1] = 0;
        return 1;
    }

    if(value < 0 && radix == 10) {
        sign = 1;
        value = -value;
    }

    while(value != 0) {
        int remainder = value % radix;
        temp[i++] = (remainder > 9) ? (remainder - 10) + 'a' : remainder + '0';
        value = value / radix;
    }

    if(sign) temp[i++] = '-';

    temp[i] = 0;

    int j, len = strlen(temp);
    for(j = 0; j < i; j++) {
        buffer[j] = temp[len - j - 1];
    }
    buffer[i] = 0;

    return i;
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

/**
 * separate_string will separate given string s into many tokens. This separation will 
 * happen at any occurrence of the separator character. The tokens will be stored in 
 * the dest, which will have to be allocated previously, as this is it's only purpose.

*/
void separate_string (const char *s, char separator, char **dest) {
    int i, j, k;

    i = 0;
    j = 0;
    k = 0;

    for(i; s[i]; i++) {
        if(s[i] != separator) {
            dest[k][j] = s[i];
            j++;
        }
        else {
            dest[k][j] = 0;
            k++;
            j = 0;
        }
    }
}
