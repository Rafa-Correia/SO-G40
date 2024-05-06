#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#ifndef MAX_STR
#define MAX_STR 301
#endif

#define PREAMBLE 10

#define REQ_CON (char)0         //REQUEST CONNECTION (through pipe)
#define REQ_EXEU (char)1        //REQUEST EXECUTE (single)
#define REQ_EXEP (char)2        //REQUEST EXECUTE (multiple)
#define REQ_STAT (char)3        //REQUEST STATUS

/**
 * handle_command will, as the name indicates, handle a request made by a client. This function receives 4 arguments, first being execution type,
 * second being the task number, third being the whole string of arguments (not separated) and fourth being the output file descriptor.
*/
void handle_commmand(unsigned char type, unsigned int task_number, const char *to_execute, int log_fd, const char* output_folder) {
    struct timeval start_time, end_time;

    if(type == REQ_EXEU) {
        int i, j, n_tokens = 2; //n_tokens starts at 2 bc it will have n tokens + 1 (where n is the number of spaces) and we need space for a NULL in the array
        for(i = 0; to_execute[i]; i++) if(to_execute[i] == ' ') n_tokens++;

        size_t s_len = strlen(to_execute);
        char **tokens = calloc(n_tokens, sizeof(char*));

        for(i = 0; i < n_tokens - 1; i++) tokens[i] = calloc(s_len, sizeof(char));
        tokens[n_tokens - 1] = NULL;

        separate_string(to_execute, ' ', tokens);

        //create path for output file
        char * output_file_path;        //[folder]/request_[num]_output.txt  ->  strlen(output_folder) + 8 + strlen(num) + 11 + 1 (null terminator)
        char numtmp[33];
        int num_len = itoa(task_number, numtmp, 10);
        size_t folder_len = strlen(output_folder);

        size_t out_len = folder_len + 8 + num_len + 11 + 1;

        output_file_path = calloc(out_len, sizeof(char));

        //create path in outfilepath
        strcpy(output_file_path, output_folder);
        strcpy(output_file_path + folder_len, "request_");
        strcpy(output_file_path + folder_len + 8, numtmp);
        strcpy(output_file_path + folder_len + 8 + num_len , "_output.txt");

        //printf("%s\n", output_file_path);

        //open outfile
        int out_fd = open(output_file_path, O_CREAT | O_WRONLY, S_IRWXU);


        int status;

        //actually executes the program
        gettimeofday(&start_time, NULL);
        int pid = fork();
        if(pid == 0) {
            dup2(out_fd, 1);
            execvp(tokens[0], tokens);
        }
        wait(&status);
        gettimeofday(&end_time, NULL);
        close(out_fd);

        unsigned int tot_time = (end_time.tv_usec - start_time.tv_usec)/1000;
        
        unsigned short len = strlen(tokens[0]);
    

        //write to log and shit
        printf("Write to log: %d, %d, %hu, %s\n", task_number, tot_time, len, tokens[0]);

        char write_to_log[PREAMBLE + MAX_STR];
        //=============TASK NUMBER============
        write_to_log[0] = (task_number) & 0xFF;
        write_to_log[1] = (task_number>>8) & 0xFF;
        write_to_log[2] = (task_number>>16) & 0xFF;
        write_to_log[3] = (task_number>>24) & 0xFF;
        //=============TOTAL  TIME============
        write_to_log[4] = (tot_time) & 0xFF;
        write_to_log[5] = (tot_time>>8) & 0xFF;
        write_to_log[6] = (tot_time>>16) & 0xFF;
        write_to_log[7] = (tot_time>>24) & 0xFF;
        //=============MESSAGE LEN============
        write_to_log[8] = (len) & 0xFF;
        write_to_log[9] = (len>>8) & 0xFF;

        strcpy(write_to_log+10, tokens[0]);

        lseek(log_fd, 0, SEEK_END);
        write(log_fd, write_to_log, 10 + len);
        
        for(i = 0; i < n_tokens - 1; i++) free(tokens[i]);
        free(tokens);
        free(output_file_path);

    }
    /*else if(type == REQ_EXEP) {
        int num_comandos = 0;
        char *commands[1];
        char *token = strtok(to_execute, "|");
        while(token != NULL && num_comandos < 1 ) {
            commands[num_comandos++] = token;
            token = strtok(NULL, "|");
        }

        char output_filename[64];
        //sprintf(output_filename, "/tmp/task_%s_output.log", pid); //not allowed
        int out_fd = open(output_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);

        int i, status;
        pid_t pid;

        for(i = 0; i < num_comandos; i++) {
            if((pid = fork()) < 0) {
                perror("Failed to fork");
                return;
            } else if(pid == 0) {
                dup2(out_fd, STDOUT_FILENO);
                dup2(out_fd, STDERR_FILENO);
                close(out_fd);
            }

            execvp(commands[0][i], commands[i]);
            perror("Failed to execvp");
            exit(EXIT_FAILURE);
        }

        while (num_comandos-- > 0) {
            wait(NULL);
        }

        gettimeofday(&start_time, NULL);
        waitpid(pid, &status, 0);
        gettimeofday(&end_time, NULL);

        int elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_usec - start_time.tv_usec) / 1000;
    }
    */

}