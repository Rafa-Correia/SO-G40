#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define REQUEST_PIPE "request_pipe"
#define MAX_TASKS 100

// pipe de leitura tem formato pid + n + mensagem

void handle_commmand(char tipo, char *pid, char *n, char *msg) {
    struct timeval start_time, end_time;

    if(strcmp(tipo, '0') == 0 ) {


    } else if(strcmp(tipo, '0') == 1) {
        int status;
        pid_t pid = fork();

        if (pid == -1) {
            perror("Failed to fork");
            return;
        } else if (pid == 0) {
            // Redirect stdout and stderr to files
            char output_filename[64];
            sprintf(output_filename, "/tmp/task_%s_output.log", pid);
            int out_fd = open(output_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
            if (out_fd == -1) {
                perror("Failed to open output file");
                exit(EXIT_FAILURE);
            }
            dup2(out_fd, STDOUT_FILENO);
            dup2(out_fd, STDERR_FILENO);
            close(out_fd);

            // Execute the command
            execvp(msg[0], msg);
            perror("Failed to execvp");
            exit(EXIT_FAILURE);
        }

        //Pai calcula o tempo

        gettimeofday(&start_time, NULL);
        waitpid(pid, &status, 0);
        gettimeofday(&end_time, NULL);

        int elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_usec - start_time.tv_usec) / 1000;
    
        //falta enviar para o client a informaçao(ou para o ficheiro ns)

    } else if(strcmp(tipo, '0') == 2) {
        int num_comandos = 0;
        char *commands[MAX_TASKS];
        char *token = strtok(msg, "|");
        while(token != NULL && num_comandos < MAX_TASKS ) {
            commands[num_comandos++] = token;
            token = strtok(NULL, "|");
        }

        char output_filename[64];
        sprintf(output_filename, "/tmp/task_%s_output.log", pid);
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

    } else if(strcmp(tipo, '0') == 3) {

        

    }

}

int main() {
    char buffer_type;
    char buffer_pid[4];
    char buffer_n[2];
    char buffer_msg[300];
    int req_fd;

    mkfifo("request_pipe", 0666);

    // Abrir o pipe de leitura
    req_fd = open(REQUEST_PIPE, O_RDONLY);
    if (req_fd < 0) {
        perror("Erro ao abrir request pipe");
        exit(1);
    }

    while(1) {
        read(req_fd, buffer_type, sizeof(char));
        read(req_fd, buffer_pid, sizeof(char)*4);
        read(req_fd, buffer_n, sizeof(char)*2);

        int length = atoi(buffer_n);

        read(req_fd, buffer_msg, length);





    }







    // Fechar pipes
    close(req_fd);

    return 0;
}