#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQ_CON (char)0         //REQUEST CONNECTION (through pipe)
#define REQ_EXEU (char)1        //REQUEST EXECUTE (single)
#define REQ_EXEP (char)2        //REQUEST EXECUTE (multiple)
#define REQ_STAT (char)3        //REQUEST STATUS

/**
 * handle_command will, as the name indicates, handle a request made by a client. This function receives 4 arguments, first being execution type,
 * second being the task number, third being the whole string of arguments (not separated) and fourth being the output file descriptor.
*/
void handle_commmand(unsigned char type, unsigned int task_number, const char *to_execute, int out_fd) {
    struct timeval start_time, end_time;

    if(type == REQ_EXEU) {
        int i, j;
        char **tokens = NULL;

        lseek(out_fd, 0, SEEK_END);
        dup2(out_fd, 1);

        execvp(tokens[0], tokens);
    }
    else if(type == REQ_EXEP) {
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

}