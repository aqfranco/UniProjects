#ifndef MANAGER_H
#define MANAGER_H

#define U_INT8_T 1 // sizeof(u_int8_t)
#define INT32_T 4  // sizeof(int32_t)
#define U_INT64_T 8 // sizeof(u_int64_t)

#define MAX_BOX_NAME 32
#define MAX_BOX 64
#define MAX_PIPE_NAME 256
#define MAX_ERROR_MSG 1024
#define CODE_ADD_BOX (u_int8_t) 3
#define CODE_RM_BOX (u_int8_t) 5
#define CODE_LIST_BOXES (u_int8_t) 7

// struct which holds a box's information
typedef struct { 
    char box_name[MAX_BOX_NAME];
    u_int64_t size;
    u_int64_t n_publishers;
    u_int64_t n_subscribers;
} box_info;

#endif