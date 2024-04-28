#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "utils.h"
//#include "request_handler.h"

#define SERVER_FIFO "sv_fifo"
#define MAX_TASKS 100 //tem de ser dado como arg

#define SCHED_DEF 1 //ASSUME DEFAULT IS TYPE 1
#define SCHED_1 1 //scheduling type 1 (todo) 

#define REQ_EXEU (char)1        //REQUEST EXECUTE (single)
#define REQ_EXEP (char)2        //REQUEST EXECUTE (multiple)
#define REQ_STAT (char)3        //REQUEST STATUS

// pipe de leitura tem formato pid + n + mensagem

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

unsigned int task_number;

int main(int argc, char **argv) {
    if(check_format(argc, argv) == 0) return 1;

    task_number = 0;

    char req_type;                  //request type
    int  req_pid;                   //requester pid
    char req_pid_buf[4];
    unsigned short req_msg_len;     //message lenght
    char buffer_msg[300];           //message buffer
    int req_fd;                     //file descriptor for server to client fifo

    mkfifo(SERVER_FIFO, 0666);

    // Abrir o pipe de leitura
    req_fd = open(SERVER_FIFO, O_RDONLY);

    if (req_fd < 0) {
        perror("Erro ao abrir request pipe");
        exit(1);
    }

    while(1) {
        read(req_fd, &req_type, 1);
        read(req_fd, req_pid_buf, 4);
        read(req_fd, &req_msg_len, 2);
        read(req_fd, buffer_msg, req_msg_len);

        req_pid = (req_pid_buf[0] << 24 | req_pid_buf[1] << 16 | req_pid_buf[2] << 8 | req_pid_buf[3]);

        int pid = fork();
        if(pid == 0) {
            char req_pid_string[33];
            itoa(req_pid, req_pid_string, 10);
            int feedback = open(req_pid_string, O_WRONLY);

            if(req_type == REQ_EXEU || req_type == REQ_EXEP) {
                task_number++;
                char sv_to_client[4];
                int nmb = task_number;
                sv_to_client[0] = (nmb >> 24) & 0xFF;
                sv_to_client[1] = (nmb >> 16) & 0xFF;
                sv_to_client[2] = (nmb >> 8)  & 0xFF;
                sv_to_client[3] = (nmb)       & 0xFF;

                write(feedback, sv_to_client, 4);

                //handle request here -- TODO
            }
            else {
                //status here
            }
            close(feedback);
            _exit(0);
        }


    }




    // Fechar pipes
    close(req_fd);

    return 0;
}