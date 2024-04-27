#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "utils.h"

#define SERVER_FIFO "sv_fifo"
#define MAX_TASKS 100 //tem de ser dado como arg

#define SCHED_DEF 1 //ASSUME DEFAULT IS TYPE 1
#define SCHED_1 1 //scheduling type 1 (todo) 

#define REQ_CON (char)0         //REQUEST CONNECTION (through pipe)
#define REQ_EXEU (char)1        //REQUEST EXECUTE (single)
#define REQ_EXEP (char)2        //REQUEST EXECUTE (multiple)
#define REQ_STAT (char)3        //REQUEST STATUS

// pipe de leitura tem formato pid + n + mensagem

void handle_commmand(char tipo, const char *msg) {
    struct timeval start_time, end_time;

    if(tipo == REQ_CON) {


    } else if(tipo == REQ_EXEU) {
        int status;
        pid_t pid = fork();

        if (pid == -1) {
            perror("Failed to fork");
            return;
        } else if (pid == 0) {
            // Redirect stdout and stderr to files
            char output_filename[64];
            sprintf(output_filename, "/tmp/task_%s_output.log", pid); //not allowed
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

    } else if(tipo == REQ_EXEP) {
        int num_comandos = 0;
        char *commands[MAX_TASKS];
        char *token = strtok(msg, "|");
        while(token != NULL && num_comandos < MAX_TASKS ) {
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

    } else if(tipo == REQ_STAT) {

        

    }

}

int check_format (int argc, char **argv) {
    if(argc != 3 && argc != 4) {
        write(STDOUT_FILENO, "Wrong number of arguments!\n", 28);
        return 0;
    }

    if(!(is_positive_integer(argv[2]))) {
        write(STDOUT_FILENO, "'parallel-tasks' field is wrongly formatted (maybe negative?)!\n", 64);
        return 0;
    }

    if(argc == 3) return 1;
    else {
        //add code here later if we decide to use different scheduling tactics
        return 1;
    }
}

int main(int argc, char **argv) {
    if(check_format(argc, argv) == 0) return 1;

    char buffer_type;
    char buffer_pid[4];
    char buffer_n[2];
    char buffer_msg[300];
    int req_fd;

    mkfifo(SERVER_FIFO, 0666);

    // Abrir o pipe de leitura
    req_fd = open(SERVER_FIFO, O_RDONLY);
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