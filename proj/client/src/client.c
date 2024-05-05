#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

#define SERVER_FIFO "sv_fifo"
#define MSG_BUF_LEN (size_t)312 //1 byte for type, 4 bytes for pid, 4 bytes expected runtime, 2 bytes msg_len, max 301 bytes for execution info (null terminated string)

#define REQ_EXEU (unsigned char)1        //REQUEST EXECUTE (single)
#define REQ_EXEP (unsigned char)2        //REQUEST EXECUTE (multiple)
#define REQ_STAT (unsigned char)3        //REQUEST STATUS
#define REQ_SHUTDOWN (unsigned char) 4   //REQUEST SERVER SHUTDOWN


/**
 * Function used to check if arguments given to client program are correctly formatted.
 * returns 0 - wrongly formatted
 * returns 1 - all good
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
    else if(strcmp(argv[1], "shutdown") == 0) type_flag = 3;
    else {
        write(STDOUT_FILENO, "Not a valid argument (", 23);
        write(STDOUT_FILENO, argv[1], strlen(argv[1]));
        write(STDOUT_FILENO, ")!\n", 4);
        return 0;
    }

    //CHECK IF CORRECT NUMBER OF ARGS FOR EXE AND STATUS
    
    if(type_flag == 1 && argc != 5) {
        write(STDOUT_FILENO, "Wrong number of arguments for execute!\n", 40);
        return 0;
    }
    
    if(type_flag == 2 && argc != 2) {
        write(STDOUT_FILENO, "Wrong number of arguments for status!\n", 39);
        return 0;
    }

    if(type_flag == 3 && argc != 2) {
        write(STDOUT_FILENO, "Wrong number of arguments for shutdown!\n", 39);
        return 0;
    }

    if(type_flag == 2 || type_flag == 3) return 1; //early return in case of status

    //FROM HERE ON OUT ONLY CHECK IF EXECUTE

    //checks if time field is correctly formatted
    if(!(is_positive_integer(argv[2]))) {
        write(STDOUT_FILENO, "'time' field is wrongly formatted (maybe negative?)!\n", 54);
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

    //=================================================================================
    //check format of program arguments
    if(check_format(argc, argv) == 0) return 1;
    //=================================================================================



    //=================================================================================
    //open request pipe (client to server)
    int request_pipe = open(SERVER_FIFO, O_WRONLY); 

    if(request_pipe < 0) { //stops if pipe cant open
        perror("open request_pipe");
        return 1;
    }
    //=================================================================================



    //=================================================================================
    //SELECT WHICH MESSAGE TYPE TO SEND
    unsigned char msg_type;
    if(strcmp(argv[1], "status") == 0) msg_type = REQ_STAT;
    else if(strcmp(argv[1], "execute") == 0 && strcmp(argv[3], "-u") == 0) msg_type = REQ_EXEU;
    else if(strcmp(argv[1], "execute") == 0 && strcmp(argv[3], "-p") == 0) msg_type = REQ_EXEP;
    else if(strcmp(argv[1], "shutdown") == 0) msg_type = REQ_SHUTDOWN;\
    //=================================================================================

    //=================================================================================
    //declare message buffer containing all info related to request

    //MAIN MESSAGE BUFFER
    unsigned char msg_buf[MSG_BUF_LEN];

    //get process id

    //PROGRAM PID
    pid_t pid = getpid();
    //=================================================================================



    //=================================================================================
    //BUILD AND SEND REQUEST
    
    msg_buf[0] = msg_type;
    
    //=========WRITE PID TO BUFFER=========
    msg_buf[1] = (pid)     &0xFF;
    msg_buf[2] = (pid>>8)  &0xFF;
    msg_buf[3] = (pid>>16) &0xFF;
    msg_buf[4] = (pid>>24) &0xFF;
    //=====================================

    unsigned int time;
    if(msg_type == REQ_STAT || msg_type == REQ_SHUTDOWN) time = 0;
    else time = (unsigned int)atoi(argv[2]);

    //=========WRITE TIME TO BUFFER=========
    msg_buf[5] = (time)     &0xFF;
    msg_buf[6] = (time>>8)  &0xFF;
    msg_buf[7] = (time>>16) &0xFF;
    msg_buf[8] = (time>>24) &0xFF;
    //======================================

    unsigned short msg_len;
    if(msg_type == REQ_STAT || msg_type == REQ_SHUTDOWN) msg_len = 0;
    else msg_len = (unsigned short)strlen(argv[4]);
    msg_buf[9] =  (msg_len)    &0xFF;
    msg_buf[10] = (msg_len>>8) &0xFF;

    unsigned short i, read_iter = 0;
    for(i = 11; i < msg_len+11; i++){
        msg_buf[i] = argv[4][read_iter];
        read_iter++;
    }

    char number[33];
    itoa(pid, number, 10); //store pid in string format

    int feedback_fifo_flag = mkfifo(number, 0666);
    
    if(feedback_fifo_flag < 0) {
        perror("mkfifo");
        return 1;
    }

    write(request_pipe, msg_buf, 11 + msg_len);
    close(request_pipe);

    //printf("Opening feedback pipe...\n");
    int feedback_pipe = open(number, O_RDONLY); //opens feedback pipe (server to client)
    //printf("Done!");
    
    if(feedback_pipe < 0) {
        perror("open feedback_pipe");
        return 1;
    }


    /**
     * READ FEEDBACK FROM SERVER
    */

    //===================================================================
    //===============================TODO================================
    //===================================================================

    if(msg_type == REQ_EXEU || msg_type == REQ_EXEP) {
        int task_number;

        //printf("Reading reply...\n");
        read(feedback_pipe, &task_number, 4);
        //printf("Done!\n");
        
        char number_buff[33];
        itoa(task_number, number_buff, 10);
        write(STDOUT_FILENO, "Task ", 6);
        write(STDOUT_FILENO, number_buff, strlen(number_buff));
        write(STDOUT_FILENO, " received!\n", 12);
    }

    else if(msg_type == REQ_STAT){
        //todo
    }
    
    close(feedback_pipe);
    unlink(number);
    return 0;
}