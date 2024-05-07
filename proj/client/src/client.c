#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

#define SERVER_FIFO "../tmp/sv_fifo"
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
    int i, j;

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

    unsigned short read_iter = 0;
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

        unsigned int task_number;
        char *msg;
        
        char nmb_buf[33];
        int nmb_len;

        unsigned char is_last_flag;
        unsigned int full_msg_len;

        //=======================================================EXECUTING TASKS======================================================
        write(STDOUT_FILENO, "\n>-------------------------------------------------------<\nExecuting\n\n", 71);

        while(1) {
            int bytes_read = read(feedback_pipe, &full_msg_len, 4);
            
            if(bytes_read <= 0) continue;

            msg = calloc(full_msg_len + 1, sizeof(char));
            read(feedback_pipe, msg, full_msg_len);
            
            is_last_flag = msg[0];

            if(is_last_flag == 2) {
                write(STDOUT_FILENO, "No tasks are executing.\n", 25);
                free(msg);
                break;
            }

            task_number  = (msg[1] | msg[2] << 8 | msg[3] << 16 | msg[4] << 24);
            msg_len      = (msg[5] | msg[6] << 8);


            //=================================WRITE TO CONSOLE (OR STDOUT)===========================================
            write(STDOUT_FILENO, "Task no. ", 10);
            
            nmb_len = itoa(task_number, nmb_buf, 10);
            write(STDOUT_FILENO, nmb_buf, nmb_len);

            write(STDOUT_FILENO, " --> ", 6);

            write(STDOUT_FILENO, msg+7, msg_len);

            write(STDOUT_FILENO, "\n", 2);

            //========================================================================================================

            free(msg);

            if(is_last_flag == 1) break; //if was last message then break.
        }


        //========================================================QUEUED TASKS========================================================
        write(STDOUT_FILENO, "\n>-------------------------------------------------------<\nQueued\n\n", 68);


        while(1) {
            int bytes_read = read(feedback_pipe, &full_msg_len, 4);
            
            if(bytes_read <= 0) continue;

            msg = calloc(full_msg_len + 1, sizeof(char));
            read(feedback_pipe, msg, full_msg_len);
            
            is_last_flag = msg[0];

            if(is_last_flag == 2) {
                write(STDOUT_FILENO, "No tasks are queued.\n", 22);
                free(msg);
                break;
            }

            task_number  = (msg[1] | msg[2] << 8 | msg[3] << 16 | msg[4] << 24);
            msg_len      = (msg[5] | msg[6] << 8);


            //=================================WRITE TO CONSOLE (OR STDOUT)===========================================
            write(STDOUT_FILENO, "Task no. ", 10);
            
            nmb_len = itoa(task_number, nmb_buf, 10);
            write(STDOUT_FILENO, nmb_buf, nmb_len);

            write(STDOUT_FILENO, " --> ", 6);

            write(STDOUT_FILENO, msg+7, msg_len);

            write(STDOUT_FILENO, "\n", 2);

            //========================================================================================================

            free(msg);

            if(is_last_flag == 1) break; //if was last message then break.
        }



        //=======================================================COMPLETED TASKS=======================================================
        write(STDOUT_FILENO, "\n>-------------------------------------------------------<\nCompleted\n\n", 71);

        int log_size;
        int bytes_read = read(feedback_pipe, &log_size, 4);

        if(bytes_read )

        if(log_size == 0) {
            write(STDOUT_FILENO, "No tasks completed yet.\n", 25);
        } 

        else {
            unsigned char* log_content = calloc(log_size + 1, sizeof(char));
            read(feedback_pipe, log_content, log_size);

            int execution_t;


            int offset = 0;
            while(offset < log_size) {
                task_number = (log_content[offset] | log_content[offset+1] << 8 | log_content[offset+2] << 16 | log_content[offset+3] << 24);
                offset+=4;
                execution_t = (log_content[offset] | log_content[offset+1] << 8 | log_content[offset+2] << 16 | log_content[offset+3] << 24);
                offset+=4;
                msg_len = (log_content[offset] | log_content[offset+1] << 8);
                offset+=2;

                msg = calloc(msg_len+1, sizeof(char));
                j=0;
                for(i = offset; i < offset + msg_len; i++) {
                    msg[j] = log_content[i];
                    j++;
                }
                offset+=msg_len;

                printf("et: %d\n", execution_t);
                //=================================WRITE TO CONSOLE (OR STDOUT)===========================================
                write(STDOUT_FILENO, "Task no. ", 10);
                nmb_len = itoa(task_number, nmb_buf, 10);
                write(STDOUT_FILENO, nmb_buf, nmb_len);

                write(STDOUT_FILENO, " --> ", 6);

                write(STDOUT_FILENO, msg, msg_len);

                write(STDOUT_FILENO, ", took ", 8);

                nmb_len = itoa(execution_t, nmb_buf, 10);
                write(STDOUT_FILENO, nmb_buf, nmb_len);

                write(STDOUT_FILENO, " ms\n", 5);
                //========================================================================================================

                free(msg);
            }
            free(log_content);
        }
    }
    
    close(feedback_pipe);
    unlink(number);
    return 0;
}