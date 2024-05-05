/**
 * This file contains all source code for general purpose functions. 
 * These functions are used in both the client program and the orquestrator program.
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

    return strlen(buffer);
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
 * the empty_storage, as this is it's only purpose. Returns the number of tokens generated.
 * Empty storage is allocated here. Will have one extra 'token' that is NULL.
*/
int separate_string (const char *s, char separator, char **empty_storage) {
    int i, j, k, tok_count = 2; //i and j are common iterators. tok_count will be return value containing number of tokens
    size_t s_len = strlen(s) + 1;
    for(i = 0; s[i]; i++) {
        if(s[i] == separator) tok_count++;
    }

    printf("separate_string:\n");
    printf("handling: %s\n", s);
    printf("tokens: %d\n", tok_count);

    empty_storage = calloc(tok_count, sizeof(char*));
    i = 0;
    while(i < tok_count - 1) {
        printf("i: %d\n", i);
        empty_storage[i] = calloc(s_len, sizeof(char));
        i++;
    }
    empty_storage[i] = NULL;

    i = 0;
    j = 0;
    k = 0;

    printf("Separating now...\n");

    for(i; s[i]; i++) {
        printf("i: %d, j: %d, k: %d, s[i]: %c\n", i, j, k, s[i]);
        if(s[i] != separator) {
            empty_storage[k][j] = s[i];
            j++;
        }
        else {
            empty_storage[k][j] = 0;
            k++;
            j = 0;
        }
    }
    printf("%s\n", empty_storage[0]);

    return tok_count;
}

