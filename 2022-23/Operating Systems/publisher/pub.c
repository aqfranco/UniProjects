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
#include "pub.h"

int pp; // publisher's pipe file descriptor
char *register_pipe, *box_name, *pub_pipe;

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stdout, "Unexpected number of arguments.\n");
        return -1;
    }
    register_pipe = argv[1];
    pub_pipe = argv[2];
    if (unlink(pub_pipe) != 0 && errno != ENOENT) {
        fprintf(stderr, "Error: unlink(%s) failed: %s\n", pub_pipe,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (mkfifo(pub_pipe, 0640) != 0) { // create pipe for communication with mbroker
        fprintf(stdout, "Error creating pipe.\n");
        return -1;
    }
    box_name = argv[3];

    // define signals's behaviour
    if (signal(SIGINT, SIG_IGN) == SIG_ERR || 
        signal(SIGQUIT, SIG_IGN) == SIG_ERR || 
        signal(SIGTERM, SIG_IGN) == SIG_ERR ||
        signal(SIGPIPE, sig_handler) == SIG_ERR) {
            exit(EXIT_FAILURE);
    }

    // register publisher in mbroker
    if (!register_publisher()) {
        fprintf(stdout, "Error initializing producer.\n");
        return -1;
    }

    pp = open(pub_pipe, O_WRONLY); // open pipe to send publications to mbroker
    size_t message_size = U_INT8_T + MAX_PUB_SIZE;
    while (true) {
        char message[U_INT8_T + MAX_PUB_SIZE], *aux = message, publication[MAX_PUB_SIZE];
        memset(publication, 0, MAX_PUB_SIZE);

        if (read(STDIN_FILENO, publication, MAX_PUB_SIZE) == -1) { // read new publication from stdin
            fprintf(stdout, "Terminating.\n"); // has read a EOF (End Of File)
            close(pp);
            if (unlink(pub_pipe) != 0) {
                fprintf(stderr, "Error: unlink(%s) failed: %s\n", pub_pipe,
                strerror(errno));
                    exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }

        u_int8_t code = CODE_SEND_PUB;
        memcpy(aux, &code, U_INT8_T); // puts code in message
        aux++;
        memcpy(aux, publication, MAX_PUB_SIZE); // puts publication in mbroker
        if (write(pp, message, message_size) != message_size) { // send new publication to mbroker
            fprintf(stdout, "Error communicating with mbroker.\n");
            return -1;
        }
    }
    return 0;
}

/**
 * Requests mbroker to register new publisher to given box.
 * 
 * Returns true if successful, false otherwise.
 * 
 */
int register_publisher() {
    int rp = open(register_pipe, O_WRONLY); // open mbroker's register pipe
    if (rp == -1 || pp == -1) {
        return false;
    }

    size_t message_size = U_INT8_T + MAX_PIPE_NAME + MAX_BOX_NAME;
    char ppipe_name[MAX_PIPE_NAME], pbox_name[MAX_BOX_NAME], 
        message[U_INT8_T + MAX_PIPE_NAME + MAX_BOX_NAME], *aux = message;
    
    memset(pbox_name, 0, MAX_BOX_NAME);
    strcpy(pbox_name, box_name);

    memset(ppipe_name, 0, MAX_PIPE_NAME);
    strcpy(ppipe_name, pub_pipe);

    u_int8_t code = CODE_REGISTER;
    memcpy(aux, &code, U_INT8_T); // puts code in message
    aux++;
    memcpy(aux, ppipe_name, MAX_PIPE_NAME); // puts publisher's pipe in message
    aux += MAX_PIPE_NAME;
    memcpy(aux, pbox_name, MAX_BOX_NAME); // put's the given box's name
    
    if (write(rp, message, message_size) != message_size) // sends request to mbroker
        return false;

    close(rp); // close mbroker register pipe
    return true;
}

/**
 * Handles the signal SIGPIPE to exit correctly.
 * 
 */
static void sig_handler(int sig) {
    if (sig == SIGPIPE) {
        if (unlink(pub_pipe) != 0) {
            fprintf(stderr, "Error: unlink(%s) failed: %s\n", pub_pipe,
                strerror(errno));
        }
        exit(EXIT_FAILURE);
    }
}