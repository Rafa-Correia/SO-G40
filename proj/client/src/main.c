#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <string.h>

#include "error_handler.h"

/**
 * Checks to see is the input of the program is correctly formatted.
*/
int check_format (int argc, char ** argv) {
    int ex_st_flag = 0;     //exec or status flag - tells the function if the program is in exec or status mode.

    if(argc != 5 && argc != 2) {                //CHECKS TO SEE IF THERE ARE ENOUGH ARGUMENTS
        print_error(ERR_ARGS_NUMBER);
        return 1;
    }



    if(strcmp(argv[1], "execute") == 0) {             //CHECKS IF SECOND ARGUMENT IS EXEC OR STATUS
        ex_st_flag = 0;
    }
    else if(strcmp(argv[1], "status") == 0) {
        ex_st_flag = 1;
    }
    else {
        print_error(ERR_ARGS_FORMAT);
        return 1;
    }



    if(argc == 2 && !ex_st_flag){           //STATUS WITH TOO MANY ARGS
        print_error(ERR_ARGS_NUMBER);
        return 1;
    }
    else if(argc == 5 && ex_st_flag){           //EXEC WITH TOO FEW ARGS
        print_error(ERR_ARGS_NUMBER);
        return 1;
    }


    return 0;
}





/**
 * Main function of the client program.
*/
int main (int argc, char ** argv) {
    if(check_format(argc, argv) != 0) return 1;




}