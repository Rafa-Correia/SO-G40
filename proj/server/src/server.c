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

#define SCHED_DEF 1     //ASSUME DEFAULT IS TYPE 1
#define SCHED_FCFS 1    //scheduling type 1 - First come first served.
#define SCHED_SJF 2     //scheduling type 2 - Shortest Job First

#define REQ_EXEU (unsigned char)1        //REQUEST EXECUTE (single)
#define REQ_EXEP (unsigned char)2        //REQUEST EXECUTE (multiple)
#define REQ_STAT (unsigned char)3        //REQUEST STATUS


//stuff related to request queue

struct request_queue {
    unsigned char type;
    unsigned int task_number;
    char * to_execute;
    struct request_queue* next;
};

struct request_queue* new_queue_node () {
    struct request_queue* new_node = malloc(sizeof(struct request_queue));
    new_node->type = 0;
    new_node->task_number = 0;
    new_node->to_execute = NULL;
    new_node->next = NULL;
    return new_node;
}

struct request_queue* get_tail (struct request_queue* head) {
    struct request_queue* tmp = head;
    while(tmp->next) tmp = tmp->next;
    return tmp;
}

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

struct request_queue* queue_head;
unsigned int task_number;

int main(int argc, char **argv) {
    int i;                          //general iterator
    int concurrent_processes;
    int max_concurrect_processes;
    queue_head == NULL;

    if(check_format(argc, argv) == 0) return 1;
    max_concurrect_processes = atoi(argv[2]);

    size_t s_len = strlen(argv[1]);

    char *output_file_path = calloc(s_len + 4, sizeof(char));
    for(i = 0; i < s_len; i++) {
        output_file_path[i] = argv[1][i];
    }
    output_file_path[s_len] = 'l';
    output_file_path[s_len+1] = 'o';
    output_file_path[s_len+2] = 'g';
    output_file_path[s_len+3] = 0;

    int out_fd = open(output_file_path, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);

    if(out_fd < 0) {
        perror("open output file");
        return 1;
    }

    lseek(out_fd, 0, SEEK_SET);

    int bytes_read = read(out_fd, &task_number, 4);
    if(bytes_read == 0) task_number = 0;

    lseek(out_fd, 0, SEEK_SET);
    write(out_fd, &task_number, 4);

    //task_number = 0;

    char req_type;                  //request type
    int  req_pid;                   //requester pid
    unsigned short req_msg_len;     //message lenght
    char buffer_msg[300];           //message buffer
    int req_fd;                     //file descriptor for server to client fifo

    mkfifo(SERVER_FIFO, 0666);

    // Abrir o pipe de leitura
    

    while(1) {
        req_fd = open(SERVER_FIFO, O_RDONLY);
        if (req_fd < 0) {
            perror("Erro ao abrir request pipe");
            return 1;
        }

        read(req_fd, &req_type, 1);
        read(req_fd, &req_pid, 4);
        read(req_fd, &req_msg_len, 2);
        read(req_fd, buffer_msg, req_msg_len);

        printf("%hu\n", req_msg_len);

        //printf("Requester pid is %d!\n", req_pid);

        char req_pid_string[33];
        itoa(req_pid, req_pid_string, 10);
        
        int feedback = open(req_pid_string, O_WRONLY);
        if(req_type == REQ_EXEU || req_type == REQ_EXEP) {
            task_number++;
            lseek(out_fd, 0, SEEK_SET);
            write(out_fd, &task_number, 4);

            //printf("Writing reply to client...\n");
            write(feedback, &task_number, 4);
            //printf("Done!\n");
            //place request in queue
            struct request_queue* new_request = new_queue_node();
            new_request->type = req_type;
            new_request->task_number = task_number;
            new_request->to_execute = malloc((size_t)req_msg_len);
            strcpy(new_request->to_execute, buffer_msg);

            if(queue_head == NULL) {
                queue_head = new_request;
            }
            else {
                struct request_queue* tail = get_tail(queue_head);
                tail->next = new_request;
            }
        }
        else {
            //status here
            break;
        }
        close(feedback);
        //_exit(0);
        

        close(req_fd);
    }




    // Fechar pipes
    close(req_fd);

    return 0;
}