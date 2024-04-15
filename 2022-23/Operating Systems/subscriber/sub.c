#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "logging.h"
#include "../utils/betterassert.h"
#include "sub.h"

int total_msg = 0; // number of messages received

int sp; // pipe for receiving publications' file descriptor

char *register_pipe, *sub_pipe, *box_name;

int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc == 4, "Unexpected number of arguments.\n");
    register_pipe = argv[1];
    sub_pipe = argv[2];
    box_name = argv[3];
    if (unlink(sub_pipe) != 0 && errno != ENOENT) {
        fprintf(stderr, "Error: unlink(%s) failed: %s\n", sub_pipe,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (mkfifo(sub_pipe, 0640) != 0) {
        fprintf(stdout, "Error creating pipe.\n");
        return -1;
    }

    // define signals' behaviour
    if (signal(SIGINT, sig_handler) == SIG_ERR || 
        signal(SIGQUIT, SIG_IGN) == SIG_ERR || 
        signal(SIGTERM, SIG_IGN) == SIG_ERR ||
        signal(SIGPIPE, sig_handler) == SIG_ERR) {
            exit(EXIT_FAILURE);
    }

    register_subscriber(); // register subscriber in mbroker

    sp = open(sub_pipe, O_RDONLY); // open pipe for receiving publications
    ALWAYS_ASSERT(sp != -1, "Error opening pipe.\n");

    while (true) {
        u_int8_t code;
        char message[MAX_MSG_SIZE];
        if (read(sp, &code, U_INT8_T) != U_INT8_T || read(sp, message, MAX_MSG_SIZE) != MAX_MSG_SIZE) { // read publication
            // read has failed (pipe was closed in mbroker's side)
            if (unlink(sub_pipe) != 0) {
                fprintf(stderr, "Error: unlink(%s) failed: %s\n", sub_pipe,
                    strerror(errno));
            }
            fprintf(stdout, "Terminating.\n");
            exit(EXIT_SUCCESS);
        }
        ALWAYS_ASSERT(code == CODE_SENT_SUB, "Error comunicating with mbroker (unexpected message code).\n");
        total_msg++;
        fprintf(stdout, "%s\n", message);
    }
    return 0;
}

/**
 * Sends a request to mbroker to register subscriber.
 */
void register_subscriber() {
    int rp = open(register_pipe, O_WRONLY); // open mbroker register pipe for writing
    ALWAYS_ASSERT(rp != -1, "Error opening pipe.\n");

    size_t message_size = U_INT8_T + MAX_PIPE_NAME + MAX_BOX_NAME;
    char message[U_INT8_T + MAX_PIPE_NAME + MAX_BOX_NAME], *aux = message;
    char ppipe_name[MAX_PIPE_NAME], pbox_name[MAX_BOX_NAME];

    memset(ppipe_name, 0, MAX_PIPE_NAME);
    memset(pbox_name, 0, MAX_BOX_NAME);

    strcpy(ppipe_name, sub_pipe);
    strcpy(pbox_name, box_name);

    u_int8_t code = CODE_REGISTER;
    memcpy(aux, &code, U_INT8_T); // puts code in message
    aux++;
    memcpy(aux, ppipe_name, MAX_PIPE_NAME); // puts pipe in message
    aux += MAX_PIPE_NAME;
    memcpy(aux, pbox_name, MAX_BOX_NAME); // puts the box's name in message

    ssize_t written = write(rp, message, message_size); // sends request to mbroker
    ALWAYS_ASSERT(written == message_size, "Error communicating with mbroker.\n");
    close(rp);
}

/**
 * Handles the signal SIGINT and SIGPIPE to exit correctly.
 * 
 */
static void sig_handler(int sig) {
    if (sig == SIGINT) {
        close(sp);
        char final_message[MAX_MSG_SIZE];
        sprintf(final_message, "Terminating: received %d messages.\n", total_msg);
        if (write(STDOUT_FILENO, final_message, strlen(final_message)+1) <= 0){
            exit(EXIT_FAILURE);
        }
        if (unlink(sub_pipe) != 0) {
            fprintf(stderr, "Error: unlink(%s) failed: %s\n", sub_pipe,
                strerror(errno));
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else if (sig == SIGPIPE) {
        if (unlink(sub_pipe) != 0) {
            fprintf(stderr, "Error: unlink(%s) failed: %s\n", sub_pipe,
                strerror(errno));
        }
        exit(EXIT_FAILURE);
    }
}