#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <string.h>
#include <stdio.h>

#define SERVER_FIFO "sv_fifo"
#define MSG_BUF_LEN (size_t)307

#define REQ_CON (char)0         //REQUEST CONNECTION (through pipe)
#define REQ_EXEU (char)1        //REQUEST EXECUTE (single)
#define REQ_EXEP (char)2        //REQUEST EXECUTE (multiple)
#define REQ_STAT (char)3        //REQUEST STATUS


int is_number (const char* str) { //maybe should be on utils?
    size_t i;
    size_t len = strlen(str);
    for(i = 0; i < len; i++) {
        if(str[i] < '0' || str[i] > '9') return 0;
    }
    return 1;
}


/**
 * Function used to check if arguments given to client program are correctly formatted.
*/
int check_format (int argc, char ** argv) {
    unsigned char type_flag = 0;


    //CHECK IF CORRECT NUMBER OF ARGUMENTS
    if(argc != 2 && argc != 5) {
        write(STDOUT_FILENO, "Wrong number of arguments!\n", 28);
        return 0;
    }

    //CHECK IF EXECUTE OR STATUS
    if(strcmp(argv[1], "execute") == 0) type_flag = 1;
    else if(strcmp(argv[1], "status") == 0) type_flag = 2;
    else {
        write(STDOUT_FILENO, "Not a valid argument (", 23);
        write(STDOUT_FILENO, argv[1], strlen(argv[1]));
        write(STDOUT_FILENO, ")!\n", 4);
        return 0;
    }

    //CHECK IF CORRECT NUMBER OF ARGS FOR EXE AND STATUS
    if(type_flag == 2 && argc != 2) {
        write(STDOUT_FILENO, "Wrong number of arguments for status!\n", 39);
        return 0;
    }
    if(type_flag == 1 && argc != 5) {
        write(STDOUT_FILENO, "Wrong number of arguments for execute!\n", 40);
        return 0;
    }

    if(type_flag == 2) return 1; //early return in case of status

    //checks if time field is correctly formatted
    if(!(is_number(argv[2]))) {
        write(STDOUT_FILENO, "'time' field is not acceptable (maybe negative?)!\n", 51);
        return 0;
    }
    
    //check if flags are -u or -p
    if(strcmp(argv[3], "-u") != 0 && strcmp(argv[3], "-p") != 0) {
        write(STDOUT_FILENO, "Flag is wrongly formatted!\n", 28);
        return 0;
    }

    if(strlen(argv[4]) > 300) {
        write(STDOUT_FILENO, "Last argument too big(>300)!\n", 30);
        return 0;
    }

    return 1;
}

/**
 * Main function of the client program.
*/
int main (int argc, char ** argv) {
    unsigned char format_flag;
    format_flag = check_format(argc, argv); //check format of args
    if(format_flag == 0) return 1;

    int fd = open(SERVER_FIFO, O_WRONLY); //open server fifo


    //SELECT WHICH MESSAGE TYPE TO SEND
    unsigned char msg_type;
    if(strcmp(argv[1], "status") == 0) msg_type = REQ_STAT;
    else if(strcmp(argv[1], "execute") == 0 && strcmp(argv[3], "-u") == 0) msg_type = REQ_EXEU;
    else if(strcmp(argv[1], "execute") == 0 && strcmp(argv[3], "-p") == 0) msg_type = REQ_EXEP;

    //MAIN MESSAGE BUFFER
    char msg_buf[MSG_BUF_LEN];

    //PROGRAM PID
    pid_t pid = getpid();

    /**
     * BUILD AND SEND CONNECTION REQUEST
    */

    msg_buf[0] = REQ_CON;
    
    //=========WRITE PID TO BUFFER=========
    msg_buf[1] = (pid>>24)&0xFF;
    msg_buf[2] = (pid>>16)&0xFF;
    msg_buf[3] = (pid>>8) &0xFF;
    msg_buf[4] = (pid)    &0xFF;
    //=====================================

    write(fd, msg_buf, 5); //send connect request

    /**
     * BUILD AND SEND ACTUAL REQUEST
    */

    msg_buf[0] = msg_type;

    //=========WRITE PID TO BUFFER=========
    msg_buf[1] = (pid>>24)&0xFF;
    msg_buf[2] = (pid>>16)&0xFF;                    //maybe pointless? change later
    msg_buf[3] = (pid>>8) &0xFF;
    msg_buf[4] = (pid)    &0xFF;
    //=====================================

    if(msg_type == REQ_STAT) {
        write(fd, msg_buf, 5);
    }
    else {
        unsigned short msg_len = (unsigned short)strlen(argv[4]);
        msg_buf[5] = (msg_len>>8)&0xFF;
        msg_buf[6] = (msg_len)   &0xFF;

        unsigned short i, read = 0;
        for(i = 7; i < msg_len+7; i++){
            msg_buf[i] = argv[4][read];
            read++;
        }

        write(fd, msg_buf, 7 + msg_len);
    }

    /**
     * READ FEEDBACK FROM SERVER
    */

    //===================================================================
    //===============================TODO================================
    //===================================================================


    return 0;
}