#ifndef SUB_H
#define SUB_H

#define U_INT8_T 1 // sizeof(u_int8_t)
#define INT32_T 4  // sizeof(int32_t)
#define U_INT64_T 8 // sizeof(u_int64_t)

#define CODE_REGISTER (u_int8_t) 2
#define CODE_SENT_SUB (u_int8_t) 10
#define MAX_BOX_NAME 32
#define MAX_PIPE_NAME 256
#define MAX_MSG_SIZE 1024

void register_subscriber();

static void sig_handler(int sig);

#endif