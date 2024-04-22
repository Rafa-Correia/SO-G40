#include <unistd.h>

#include "error_handler.h"


/**
 * 1 - Wrong number of arguments.
 * 2 - Arguments formatted incorrectly.
*/


/**
 * For the given error id given, print error specifics in console (or stdout).
*/
void print_error (const unsigned int err_number) {
    switch(err_number) {
        case ERR_ARGS_NUMBER:
            write(STDOUT_FILENO, "ERR:\n\tWrong number of arguments.\n", 34);
            break;
        case ERR_ARGS_FORMAT:
            write(STDOUT_FILENO, "ERR:\n\tArguments wrongly formatted.\n", 35);
            break;
        default:
            write(STDOUT_FILENO, "ERR NOT DEFINED\n", 17);
            break;
    }
}