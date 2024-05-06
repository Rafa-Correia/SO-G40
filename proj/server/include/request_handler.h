#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

    void handle_commmand(unsigned char type, unsigned int task_number, const char *to_execute, int out_fd, const char* output_folder);

#endif