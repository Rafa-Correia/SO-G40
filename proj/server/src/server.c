#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "utils.h"
#include "request_handler.h"

#define SERVER_FIFO "../tmp/sv_fifo"
#define PROC_COUNT_SAFE_MAX 256 //tem de ser dado como arg
#define MSG_BUF_LEN (size_t)312

#define SCHED_DEF (unsigned char)1     //ASSUME DEFAULT IS TYPE 1 - FCFS
#define SCHED_FCFS (unsigned char)1    //scheduling type 1 - First come first served.
#define SCHED_SJF (unsigned char)2     //scheduling type 2 - Shortest Job First

#define REQ_EXEU (unsigned char)1           //REQUEST EXECUTE (single)
#define REQ_EXEP (unsigned char)2           //REQUEST EXECUTE (multiple)
#define REQ_STAT (unsigned char)3           //REQUEST STATUS

#define REQ_SHUTDOWN (unsigned char)4       //REQUEST SERVER SHUTDOWN         


#define STATUS_PREAMBLE 7 //is_last_flag(1 byte) + task_number(4 bytes) + cmd_len(2 bytes)

//==============================================REQUEST QUEUE STUFF====================================================
/**
 * request queue structure
*/
struct request_queue {
    unsigned char type;   //exec unique or exec pipeline
    unsigned int task_number;   //specific task number
    unsigned int expected_execution_time;   //expected runtime (for scheduling)
    char * to_execute;  //string containing all arguments
    struct request_queue* next;
};


/**
 * returns new, empty request queue node
*/
struct request_queue* new_queue_node () {
    struct request_queue* new_node = malloc(sizeof(struct request_queue));
    new_node->type = 0;
    new_node->task_number = 0;
    new_node->to_execute = NULL;
    new_node->next = NULL;
    return new_node;
}

/**
 * frees single queue node
*/
void free_queue_node (struct request_queue* to_free) {
    if(to_free == NULL) {
        return;
    }
    free(to_free->to_execute);
    free(to_free);
}


/**
 * frees entire queue
*/
void free_queue (struct request_queue* to_free_head) {
    struct request_queue* tmp;
    while(to_free_head) {
        tmp = to_free_head->next;
        free(to_free_head);
        to_free_head = tmp;
    }
}

/**
 * get last element of queue of given head
*/
struct request_queue* get_tail (struct request_queue* head) {
    struct request_queue* tmp = head;
    while(tmp->next) tmp = tmp->next;
    return tmp;
}

/**
 * purely for debug. Prints table contents
*/
void print_queue(struct request_queue *head) {
    if(head==NULL) return;
    else {
        printf("Task no. %d -> type: %d, to_execute: %s\n", head->task_number, head->type, head->to_execute);
        print_queue(head->next);
    }
}

void print_queue_node(struct request_queue *head) {
    if(head==NULL) {
        printf("Node is empty.\n");
    }
    else {
        printf("Task no. %d -> type: %d, to_execute: %s\n", head->task_number, head->type, head->to_execute);
    }
}
//========================================================================================================================



//========================================================================================================================
/**
 * Checks format of given argc and argv. Used exclusively in server's main().
*/
int check_format (int argc, char **argv) {

    //CHECK IF CORRECT NMB OF ARGS
    if(argc != 3 && argc != 4) {
        write(STDOUT_FILENO, "Wrong number of arguments!\n", 28);
        return 0;
    }



    //CHECK IF MAX PARALLEL TASKS IS POSITIVE INT
    if(!(is_positive_integer(argv[2]))) {
        write(STDOUT_FILENO, "'parallel-tasks' field is wrongly formatted (maybe negative?)!\n", 64);
        return 0;
    }

    if(argc == 3) return 1;//early return in case no sched type declared



    //CHECK IF SCHED TYPE IS VALID
    else {
        //add code here later if we decide to use different scheduling tactics
        return 1;
    }

    return 0;
}
//========================================================================================================================

struct request_queue* queued_tasks_head;
unsigned int task_number;


/**
 * main function in server program
*/
int main(int argc, char **argv) {

    //=================================================================================
    //declaring essential variables to be used later in the program
    int i, j;                       //general iterator
    int concurrent_processes = 0;       //current amount of opened processes
    int max_concurrect_processes;   //maximum ammount of opened processes
    unsigned char sched_type = SCHED_DEF;   //scheduling type

    //initialize queue as empty
    queued_tasks_head == NULL;

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
    int log_size;

    int bytes_read = read(log_fd, &task_number, 4);
    if(bytes_read <= 0) task_number = 0;
    bytes_read = read(log_fd, &log_size, 4);
    if(bytes_read <= 0) log_size = 0;

    lseek(log_fd, 0, SEEK_SET);
    write(log_fd, &task_number, 4);
    write(log_fd, &log_size, 4);
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
    unsigned char *request_buffer = NULL;                                               //buffer holding all request info
    unsigned char request_type;                                                         //request type(exec single, plural, request status or request shutdown)
    unsigned int requester_pid;                                                         //requester pid
    unsigned int request_exp_time;                                                      //request expected exec time (used in scheduling tactic)
    unsigned short req_msg_len;                                                         //request command and args length
    char * to_execute = NULL;                                                           //buffer holding request command and args
    //=================================================================================

    concurrent_processes = 0;

    int child_pids[max_concurrect_processes];
    struct request_queue* in_execution[max_concurrect_processes];
    for(i = 0; i < max_concurrect_processes; i++) {
        child_pids[i] = 0;
        in_execution[i] = NULL;
    }
    


    while(1) {
        //=============================================================================
        //communicate with child processes for request execution
        //EXECUTE REQUESTS
        if(concurrent_processes < max_concurrect_processes && queued_tasks_head != NULL) {
            for(i = 0; child_pids[i]; i++); //get first slot available
            in_execution[i] =  queued_tasks_head;
            queued_tasks_head = queued_tasks_head->next;
            int pid = fork();
            if(pid == 0) {  //child
                handle_commmand(in_execution[i]->type, in_execution[i]->task_number, in_execution[i]->to_execute, log_fd, argv[1]);
                _exit(0);
            }
            //parent
            child_pids[i] = pid;
            concurrent_processes++;


            //printf("With i: %d, created child %d, to execute %s. (now %d concurrent processes).\n", i, pid, in_execution[i]->to_execute, concurrent_processes);
            //print_queue_node(in_execution[i]);
        }

        if(concurrent_processes > 0) {
            for(i = 0; i < max_concurrect_processes; i++) {
                //printf("Checking %d...\n", i);
                if(child_pids[i] == 0) continue;
                int wait_value = waitpid(child_pids[i], NULL, WNOHANG);
                if(wait_value > 0) {
                    concurrent_processes--;
                    //printf("Child %d (%d), executing %s, has finished executing (now %d concurrent processes).\n", i, child_pids[i], in_execution[i]->to_execute, concurrent_processes);
                    child_pids[i] = 0;
                    //print_queue_node(in_execution[i]);

                    struct request_queue* to_free = in_execution[i];
                    in_execution[i] = NULL;
                    free_queue_node(to_free);
                    //print_queue_node(in_execution[i]);
                }
            }
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

            //write task number to client
            write(feedback, &task_number, 4);

            //should only write to file if it executed- TODO
            lseek(log_fd, 0, SEEK_SET);
            write(log_fd, &task_number, 4);

            //create new request and set values accordingly
            struct request_queue* new_request = new_queue_node();
            new_request->type = request_type;
            new_request->task_number = task_number;
            new_request->expected_execution_time = request_exp_time;
            new_request->to_execute = calloc(req_msg_len + 1, sizeof(char));
            strcpy(new_request->to_execute, to_execute);

            //place request on queue
            if(queued_tasks_head == NULL) queued_tasks_head = new_request;
            else {
                if(sched_type == SCHED_FCFS) {//first come first served
                    struct request_queue* tail = get_tail(queued_tasks_head);
                    tail->next = new_request;
                }
            }
        }

        //in case of status request, handle status
        else if(request_type == REQ_STAT) {
            int fork_pid = fork();
            if(fork_pid == 0) {
                
                unsigned char is_last_flag;     //if block of data is last then is_last_flag = 1, if queue is empty is_last_flag = 2 <-- ONLY USED IF EXECUTING OR QUEUED
                unsigned int status_task;       //task number of task in queue
                unsigned short cmd_len;         //queued command length
                char * msg;                     //buffer to hold full message to client

                //==============================EXECUTING================================

                for(i = 0; in_execution[i] == NULL && i < max_concurrect_processes; i++);
                if(i >= max_concurrect_processes) {
                    is_last_flag = 2;
                    int one = 1;
                    write(feedback, &one, 4);
                    write(feedback, &is_last_flag, 1);
                }
                
                else{
                    j = 0;
                    for(i = 0; i < max_concurrect_processes; i++) if(in_execution[i] != NULL) j = i;
                    //J STORES VALUE OF LARGEST I THAT ISNT EMPTY
                    for(i = 0; i < max_concurrect_processes; i++) {
                        if(in_execution[i] == NULL) continue;
                        if(i == j) is_last_flag = 1;
                        else is_last_flag = 0;
                        status_task = in_execution[i]->task_number;
                        cmd_len = strlen(in_execution[i]->to_execute);
                        msg = calloc(STATUS_PREAMBLE + cmd_len + 1, sizeof(char));

                        //==============LAST FLAG=============
                        msg[0] = is_last_flag;
                        //=============TASK NUMBER============
                        msg[1] = (status_task) & 0xFF;
                        msg[2] = (status_task>>8) & 0xFF;
                        msg[3] = (status_task>>16) & 0xFF;
                        msg[4] = (status_task>>24) & 0xFF;
                        //=============MESSAGE LEN============
                        msg[5] = (cmd_len) & 0xFF;
                        msg[6] = (cmd_len>>8) & 0xFF;

                        strcpy(msg + STATUS_PREAMBLE, in_execution[i]->to_execute);

                        int full_msg_len = STATUS_PREAMBLE+cmd_len;
                        write(feedback, &full_msg_len, 4);

                        write(feedback, msg, STATUS_PREAMBLE+cmd_len);

                        free(msg);

                    }
                }
                
                //=======================================================================



                //===============================QUEUED==================================
                //printf("Sending queued tasks...\n");

                struct request_queue *tmp = queued_tasks_head;

                if(tmp == NULL) {
                    //printf("Queue is empty.\n");
                    is_last_flag = 2;
                    int one = 1;
                    write(feedback, &one, 4);
                    write(feedback, &is_last_flag, 1);
                }
                else {
                    while(tmp) {
                        is_last_flag = 0;
                        if(tmp->next == NULL) is_last_flag = 1;
                        status_task = tmp->task_number;
                        cmd_len = strlen(tmp->to_execute);
                        msg = calloc(STATUS_PREAMBLE + cmd_len + 1, sizeof(char));

                        //==============LAST FLAG=============
                        msg[0] = is_last_flag;
                        //=============TASK NUMBER============
                        msg[1] = (status_task) & 0xFF;
                        msg[2] = (status_task>>8) & 0xFF;
                        msg[3] = (status_task>>16) & 0xFF;
                        msg[4] = (status_task>>24) & 0xFF;
                        //=============MESSAGE LEN============
                        msg[5] = (cmd_len) & 0xFF;
                        msg[6] = (cmd_len>>8) & 0xFF;

                        strcpy(msg + STATUS_PREAMBLE, tmp->to_execute);

                        int full_msg_len = STATUS_PREAMBLE+cmd_len;
                        write(feedback, &full_msg_len, 4);

                        write(feedback, msg, STATUS_PREAMBLE+cmd_len);

                        free(msg);

                        tmp = tmp->next;
                    }
                }   


                //=======================================================================



                //=============================COMPLETED=================================
                //printf("Sending completed tasks...\n");
                lseek(log_fd, 4, SEEK_SET);
                read(log_fd, &log_size, 4);

                write(feedback, &log_size, 4);

                char *log_content = calloc(log_size + 1, sizeof(char));
                int bytes_read = read(log_fd, log_content, log_size);

                write(feedback, log_content, log_size);

                free(log_content);
                //=======================================================================

                
                _exit(0);
            }
        }

        else if(request_type == REQ_SHUTDOWN) {
            close(feedback);
            free(request_buffer);
            break;
        }
        close(feedback);
        //=============================================================================
        
    }
    //=================================================================================



    //=================================================================================
    //on closing, free all allocated memory and close pipes (deleting request pipe)
    free_queue(queued_tasks_head);
    // Fechar pipes
    close(req_fd);
    close(log_fd);
    unlink(SERVER_FIFO);
    //=================================================================================

    return 0;
}