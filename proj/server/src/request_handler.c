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

#define MAX_PROG_COUNT 300
#define MAX_ARG_COUNT 300

#define PREAMBLE 10

#define REQ_EXEU (char)1        //REQUEST EXECUTE (single)
#define REQ_EXEP (char)2        //REQUEST EXECUTE (multiple)

/**
 * handle_command will, as the name indicates, handle a request made by a client. This function receives 4 arguments, first being execution type,
 * second being the task number, third being the whole string of arguments (not separated) and fourth being the output file descriptor.
*/
void handle_commmand(unsigned char type, unsigned int task_number, const char *to_execute, int log_fd, const char* output_folder) {
    struct timeval start_time, end_time;

    if(type == REQ_EXEU) {
        int i, j, n_tokens = 2; //n_tokens starts at 2 bc it will have n tokens + 1 (where n is the number of spaces) and we need space for a NULL in the array


        //===========================================================================================================================================
        size_t s_len = strlen(to_execute);
        char **tokens = calloc(n_tokens, sizeof(char*));

        char* tmp = calloc(s_len+1, sizeof(char));
        strcpy(tmp, to_execute);
        remove_extremity_spaces(tmp);

        for(i = 0; tmp[i]; i++) if(tmp[i] == ' ') n_tokens++;


        for(i = 0; i < n_tokens - 1; i++) tokens[i] = calloc(s_len, sizeof(char));
        tokens[n_tokens - 1] = NULL;

        separate_string(tmp, ' ', tokens);

        free(tmp);
        //===========================================================================================================================================



        //===========================================================================================================================================
        //create path for output file and open output file
        char * output_file_path;        //[folder]/request_[num]_output.txt  ->  strlen(output_folder) + 8 + strlen(num) + 11 + 1 (null terminator)
        char numtmp[33];
        int num_len = itoa(task_number, numtmp, 10);
        size_t folder_len = strlen(output_folder);

        size_t out_len = folder_len + 8 + num_len + 11 + 1; //these numbers are above! line 44 comment

        output_file_path = calloc(out_len, sizeof(char));

        //create path in outfilepath
        strcpy(output_file_path, output_folder);
        strcpy(output_file_path + folder_len, "request_");
        strcpy(output_file_path + folder_len + 8, numtmp);
        strcpy(output_file_path + folder_len + 8 + num_len , "_output.txt");

        //printf("%s\n", output_file_path);

        //open outfile
        int out_fd = open(output_file_path, O_CREAT | O_WRONLY, S_IRWXU);
        //===========================================================================================================================================



        //===========================================================================================================================================
        //actually executes the program
        gettimeofday(&start_time, NULL);
        int pid = fork();
        if(pid == 0) {
            dup2(out_fd, 1);
            execvp(tokens[0], tokens);
            perror("execvp");
            _exit(1);
        }
        else {
            wait(NULL);
        }
        gettimeofday(&end_time, NULL);
        close(out_fd);
        //===========================================================================================================================================

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

        strcpy(write_to_log+PREAMBLE, tokens[0]);

        lseek(log_fd, 0, SEEK_END);
        write(log_fd, write_to_log, PREAMBLE + len);
        
        for(i = 0; i < n_tokens - 1; i++) free(tokens[i]);
        free(tokens);
        free(output_file_path);

    }

    else if(type == REQ_EXEP) {
        int i, j, k, cmd_count = 1;

        char * *large_tokens/*[MAX_PROG_COUNT]*/;
        char * **final_tokens/*[MAX_PROG_COUNT][MAX_ARG_COUNT]*/;

        

        //===========================================================================================================================================
        //allocate memory necessary for large_tokens and fill with separated tokens

        char *tmp = calloc(strlen(to_execute) + 1, sizeof(char));
        strcpy(tmp, to_execute);
        remove_extremity_spaces(tmp);

        for(i = 0; tmp[i]; i++) if(tmp[i] == '|') cmd_count++;  //count number of large tokens (or tokens separated by |)
        
        large_tokens = calloc(cmd_count, sizeof(char*)); 
        for(i = 0; i < cmd_count; i++) large_tokens[i] = calloc(MAX_STR, sizeof(char));

        separate_string(tmp, '|', large_tokens);
        for(i = 0; i < cmd_count; i++) {
            remove_extremity_spaces(large_tokens[i]);
        }

        free(tmp);
        //===========================================================================================================================================



        //===========================================================================================================================================
        //allocate memory necessary for final_tokens and fill with separated tokens
        final_tokens = calloc(cmd_count, sizeof(char**));
        for(i = 0; i < cmd_count; i++) {
            int small_tok_count = 2;
            for(j = 0; large_tokens[i][j]; j++) if(large_tokens[i][j] == ' ') small_tok_count++;//count number of tokens
            final_tokens[i] = calloc(small_tok_count, sizeof(char*));//alocate part of memory
            for(j = 0; j < small_tok_count-1; j++) final_tokens[i][j] = calloc(MAX_STR, sizeof(char));//allocate rest of memory
            final_tokens[i][small_tok_count - 1] = NULL;//last tokens are alway null terminated
            separate_string(large_tokens[i], ' ', final_tokens[i]);
            free(large_tokens[i]);
        }
        free(large_tokens);

        for(i = 0; i < cmd_count; i++) {
            printf("\n%d->\n", i);
            for(j = 0; final_tokens[i][j]; j++) {
                printf("\t%d -> %s\n", j, final_tokens[i][j]);
            }
        }

        //===========================================================================================================================================
        


        //==========================================================OPEN OUTPUT FILE=================================================================
        char * output_file_path;        //[folder]/request_[num]_output.txt  ->  strlen(output_folder) + 8 + strlen(num) + 11 + 1 (null terminator)
        char numtmp[33];
        int num_len = itoa(task_number, numtmp, 10);
        size_t folder_len = strlen(output_folder);

        size_t out_len = folder_len + 8 + num_len + 11 + 1; //these numbers are above! line 44 comment

        output_file_path = calloc(out_len, sizeof(char));

        //create path in outfilepath
        strcpy(output_file_path, output_folder);
        strcpy(output_file_path + folder_len, "request_");
        strcpy(output_file_path + folder_len + 8, numtmp);
        strcpy(output_file_path + folder_len + 8 + num_len , "_output.txt");

        //printf("%s\n", output_file_path);

        //open outfile
        int out_fd = open(output_file_path, O_CREAT | O_WRONLY, S_IRWXU);

        free(output_file_path);
        //===========================================================================================================================================
        




        //===========================================================================================================================================
        //declare important variables
        int pid, in_fd, pipe_fd[2];
        //===========================================================================================================================================

        for(i = 0; i < cmd_count; i++) {
            
        }

        //while(wait(NULL) > 0);
        //printf("All done!\n");
    }

}