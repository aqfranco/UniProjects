#ifndef MBROKER_H
#define MBROKER_H

#define U_INT8_T 1 // sizeof(u_int8_t)
#define INT32_T 4  // sizeof(int32_t)
#define U_INT64_T 8 // sizeof(u_int64_t)

#define LAST (u_int8_t) 1
#define RETURN_0 (int32_t) 0
#define RETURN_ERROR (int32_t) -1

#define REGISTER_PUB (u_int8_t) 1
#define REGISTER_SUB (u_int8_t) 2
#define REGISTER_BOX (u_int8_t) 3
#define CODE_ADD_BOX (u_int8_t) 4
#define REMOVE_BOX (u_int8_t) 5
#define CODE_RM_BOX (u_int8_t) 6
#define LIST_BOXES (u_int8_t) 7
#define CODE_LIST_BOXES (u_int8_t) 8
#define CODE_RECEIVE_PUB (u_int8_t) 9
#define CODE_SEND_PUB (u_int8_t) 10

#define MAX_BOX_NAME 32
#define MAX_PIPE_NAME 256
#define MAX_ERROR_MSG 1024
#define MAX_PUB_SIZE 1024

// struct that defines a box
typedef struct {
    char name[MAX_BOX_NAME];
    u_int64_t has_publisher;
    u_int64_t n_subscribers;

    int deleted;
    pthread_mutex_t message_lock;
    pthread_cond_t message_condvar;

    pthread_t publisher;
} box;

// box's list entry
typedef struct b_list {
    box tbox;
    struct b_list *next;
} *box_list;

// struct which has arguments for worker threads
typedef struct {
    u_int8_t code;
    char client_pipe[MAX_PIPE_NAME];
    char box_name[MAX_BOX_NAME];
} command_args;

int add_to_box_list(box new_box);
int remove_from_box_list(char *box_name);
box_list assoc_pub(char *box_name);
box_list assoc_sub(char *box_name);

char *get_file_name(char *box_name);

void *wait_for_command();
int put_in_queue(u_int8_t code);

void register_publisher(command_args *arguments);
void register_subscriber(command_args *arguments);
void register_box(command_args *arguments);
void remove_box(command_args *arguments);
void list_boxes(command_args *arguments);

static void sig_handler(int sig);

#endif