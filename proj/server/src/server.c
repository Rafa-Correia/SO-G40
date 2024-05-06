#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "request_handler.h"


#define SERVER_FIFO "sv_fifo"
#define PROC_COUNT_SAFE_MAX 256 //tem de ser dado como arg
#define MSG_BUF_LEN (size_t)312

#define SCHED_DEF (unsigned char)1     //ASSUME DEFAULT IS TYPE 1 - FCFS
#define SCHED_FCFS (unsigned char)1    //scheduling type 1 - First come first served.
#define SCHED_SJF (unsigned char)2     //scheduling type 2 - Shortest Job First

#define REQ_EXEU (unsigned char)1           //REQUEST EXECUTE (single)
#define REQ_EXEP (unsigned char)2           //REQUEST EXECUTE (multiple)
#define REQ_STAT (unsigned char)3           //REQUEST STATUS

#define REQ_SHUTDOWN (unsigned char)4       //REQUEST SERVER SHUTDOWN         


//stuff related to request queue

struct request_queue {
    unsigned char type;
    unsigned int task_number;
    unsigned int expected_execution_time;
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

void free_queue_node (struct request_queue* to_free) {
    if(to_free == NULL) {
        printf("NULL!!!!!!!!!!\n");
        return;
    }
    free(to_free->to_execute);
    free(to_free);
}

void free_queue (struct request_queue* to_free_head) {
    struct request_queue* tmp;
    while(to_free_head) {
        tmp = to_free_head->next;
        free(to_free_head);
        to_free_head = tmp;
    }
}

struct request_queue* get_tail (struct request_queue* head) {
    struct request_queue* tmp = head;
    while(tmp->next) tmp = tmp->next;
    return tmp;
}

void print_queue(struct request_queue *head) {
    if(head==NULL) return;
    else {
        printf("Task no. %d -> type: %d, to_execute: %s\n", head->task_number, head->type, head->to_execute);
        print_queue(head->next);
    }
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


/**
 * main function in server program
*/
int main(int argc, char **argv) {

    //=================================================================================
    //declaring essential variables to be used later in the program
    int i;                          //general iterator
    int concurrent_processes;       //current amount of opened processes
    int max_concurrect_processes;   //maximum ammount of opened processes
    unsigned char sched_type = SCHED_DEF;   //scheduling type
    int procs_pipe_fd[PROC_COUNT_SAFE_MAX][2];

    //initialize queue as empty
    queue_head == NULL;

    //check format of arguments and terminate if not valid
    if(check_format(argc, argv) == 0) return 1;
    max_concurrect_processes = atoi(argv[2]);
    //=================================================================================



    //=================================================================================
    //open log file, if doesnt exist create it

    char *log_fpath = calloc(6, sizeof(char));
    strcpy(log_fpath, "./log");

    int log_fd = open(log_fpath, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);

    free(log_fpath);

    if(log_fd < 0) { //in case of open error terminate program
        perror("open log file");
        return 1;
    }
    //=================================================================================



    //=================================================================================
    //read first 4 bytes of log file
    //these contain the task number last stored by the server
    lseek(log_fd, 0, SEEK_SET);

    int bytes_read = read(log_fd, &task_number, 4);
    if(bytes_read <= 0) task_number = 0;

    lseek(log_fd, 0, SEEK_SET);
    write(log_fd, &task_number, 4);
    lseek(log_fd, 0, SEEK_END);
    //=================================================================================


    //=================================================================================
    //open request pipe for client to server comunication
    int req_fd;                     //file descriptor for server to client fifo
    mkfifo(SERVER_FIFO, 0666);

    // Abrir o pipe de leitura
    req_fd = open(SERVER_FIFO, O_RDONLY | O_NONBLOCK);
    if (req_fd < 0) {   //in case of open error terminate program
        perror("Erro ao abrir request pipe");
        return 1;
    }
    //=================================================================================



    //=================================================================================
    //declare essential variables to read from request pipe
    unsigned char *request_buffer = NULL;
    unsigned char request_type;
    unsigned int requester_pid;
    unsigned int request_exp_time;
    unsigned short req_msg_len;
    char * to_execute = NULL;
    //=================================================================================



    //=================================================================================
    while(1) {
        //=============================================================================
        //communicate with child processes for request execution
        if(queue_head != NULL) {
            print_queue(queue_head);
            handle_commmand(queue_head->type, queue_head->task_number, queue_head->to_execute, log_fd, argv[1]);
            struct request_queue *tmp = queue_head->next;
            free_queue_node(queue_head);
            queue_head = tmp;
        }
        //=============================================================================



        //=============================================================================
        //read request from pipe, if didnt read, continue loop
        free(request_buffer);
        request_buffer = calloc(MSG_BUF_LEN, sizeof(char));
        int bytes_read = read(req_fd, request_buffer, 312);

        if(bytes_read <= 0) {
            continue;
        }
        //=============================================================================



        //=============================================================================
        //move info from buffer to adequate variables

        request_type = request_buffer[0];                                                                                       //type
        requester_pid = (request_buffer[1] | request_buffer[2] << 8 | request_buffer[3] << 16 | request_buffer[4] << 24);       //pid
        request_exp_time = (request_buffer[5] | request_buffer[6] << 8 | request_buffer[7] << 16 | request_buffer[8] << 24);    //request expected exec time
        req_msg_len = (request_buffer[9] | request_buffer[10] << 8);                                                            //request msg length                                                      
        if(request_type == REQ_EXEP || request_type == REQ_EXEU) {
            to_execute = calloc(req_msg_len + 1, sizeof(char));                                                                     //allocate memory for execution request msg
            strcpy(to_execute, request_buffer + 11);    
        }                                                                            //copy from buffer to alocated space
        //=============================================================================

        //debug
        //printf("%d, %d, %d, %hu, %s\n", request_type, requester_pid, request_exp_time, req_msg_len, to_execute);

        //=============================================================================
        //open feedback pipe for request info (server to client)
        char req_pid_string[33];
        itoa(requester_pid, req_pid_string, 10);

        int feedback = open(req_pid_string, O_WRONLY);
        //=============================================================================



        //=============================================================================
        //in case of exec request, place request on queue. 
        if(request_type == REQ_EXEU || request_type == REQ_EXEP) {
            task_number++;

            write(feedback, &task_number, 4);
            close(feedback);

            lseek(log_fd, 0, SEEK_SET);
            write(log_fd, &task_number, 4);

            struct request_queue* new_request = new_queue_node();
            new_request->type = request_type;
            new_request->task_number = task_number;
            new_request->expected_execution_time = request_exp_time;
            new_request->to_execute = calloc(req_msg_len + 1, sizeof(char));
            strcpy(new_request->to_execute, to_execute);


            if(queue_head == NULL) queue_head = new_request;
            else {
                if(sched_type == SCHED_FCFS) {
                    struct request_queue* tail = get_tail(queue_head);
                    tail->next = new_request;
                }
            }
        }
        //in case of status request, handle status
        else if(request_type == REQ_STAT) {
            print_queue(queue_head);
            //break;
        }
        else if(request_type == REQ_SHUTDOWN) {
            free(request_buffer);
            break;
        }
        //=============================================================================
        
    }
    //=================================================================================



    //=================================================================================
    //on closing, free all allocated memory and close pipes (deleting request pipe)
    free_queue(queue_head);
    // Fechar pipes
    close(req_fd);
    unlink(SERVER_FIFO);
    //=================================================================================

    return 0;
}