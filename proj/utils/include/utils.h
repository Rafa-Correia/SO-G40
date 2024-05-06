#ifndef UTILS_H
#define UTILS_H

    int itoa(int value, char *buffer, int radix);

    int is_positive_integer (const char* str);

    void separate_string (const char *s, char separator, char **dest);

    void remove_extremity_spaces(char *str);

#endif