#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "logging.h"
#include "../utils/betterassert.h"
#include "manager.h"

char *register_pipe; // name of register pipe connected to mbroker
char *manager_pipe; // name of pipe for receiving communication from mbroker

/**
 * Function to compares boxes.
 * 
 * They are compared by their names.
 * 
 */
int cmpbox (const void * a, const void * b) { 
    box_info *box1 = (box_info*) a;
    box_info *box2 = (box_info*) b;
    return strcmp(box1->box_name, box2->box_name);
}

/*
 * Function used to either add or remove a box in mbroker.
 *
 * Depending on the value passed in code_send, it can indicate
 * an addition (code = 3) or a removal (code = 5).
 * 
 */
void communicate(char *box_name, u_int8_t code_send) {
    int rp = open(register_pipe, O_WRONLY); // open mbroker register pipe for writing
    if (rp == -1) {
        fprintf(stdout, "Error opening pipe\n");
        return;
    }

    char pbox_name[MAX_BOX_NAME], ppipe_name[MAX_PIPE_NAME];
    size_t message_size = U_INT8_T + MAX_PIPE_NAME + MAX_BOX_NAME;

    memset(pbox_name, 0, MAX_BOX_NAME);
    strcpy(pbox_name, box_name);

    memset(ppipe_name, 0, MAX_PIPE_NAME);
    strcpy(ppipe_name, manager_pipe);

    char message[U_INT8_T + MAX_PIPE_NAME + MAX_BOX_NAME], *aux = message; 

    memcpy(aux, &code_send, U_INT8_T); // puts message code in message
    aux++;
    memcpy(aux, ppipe_name, MAX_PIPE_NAME); // puts the communication pipe's name in message
    aux = aux + MAX_PIPE_NAME;
    memcpy(aux, pbox_name, MAX_BOX_NAME); // puts the given box name in message

    ssize_t wr = write(rp, message, message_size); // sends message to mbroker
    close(rp); // close mbroker register pipe

    int mp = open(manager_pipe, O_RDONLY); // open pipe for receiving communications from mbroker 
    if (mp == -1) {
        fprintf(stdout, "Error opening pipe\n");
        return;
    }

    if (wr != message_size) {
        close(mp);
        fprintf(stdout, "Error communicating with mbroker.\n");
        return;
    }

    u_int8_t code_received;
    int32_t return_code;
    char error[MAX_ERROR_MSG];

    if (read(mp, &code_received, U_INT8_T) != U_INT8_T || read(mp, &return_code, INT32_T) != INT32_T ||
        read(mp, error, MAX_ERROR_MSG) != MAX_ERROR_MSG) { // read response from mbroker
        fprintf(stdout, "Error communicating with mbroker.\n");
        close(mp); // close manager pipe
        return;
    }
    close(mp);

    ALWAYS_ASSERT(code_received == code_send + 1, "Error while managing boxes.\n")

    if (return_code == -1) { // action has failed in server
        fprintf(stdout,"ERROR %s", error);
    }
    else { // action successful
        fprintf(stdout, "OK\n");
    }

    if (unlink(manager_pipe) != 0) {
        fprintf(stderr, "Error: unlink(%s) failed: %s\n", manager_pipe,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/*
 * Function that communicates the creation of a new box.
 *
 * Argument "box_name" is the name of the newly added box.
 * 
 */
void create(char *box_name) {
    communicate(box_name, CODE_ADD_BOX);
}

/*
 * Function that communicates the removal of a box.
 *
 * The argument "box_name" is the name of the box to be removed.
 * 
 */
void delete(char *box_name) {
    communicate(box_name, CODE_RM_BOX);
}

/*
 * Function that lists all the boxes in mbroker.
 *
 * Mbroker passes all the boxes' information through a given pipe, 
 * and the boxes are listed in stdout in alphabetical order.
 * 
 */
void list_boxes() {
    int rp = open(register_pipe, O_WRONLY); // open mbroker register pipe for writing
    if (rp == -1) {
        fprintf(stdout, "Error opening pipe\n");
        return;
    }

    size_t message_size = MAX_PIPE_NAME + U_INT8_T; 

    char ppipe_name[MAX_PIPE_NAME];
    memset(ppipe_name, 0, MAX_PIPE_NAME);
    strcpy(ppipe_name, manager_pipe);

    char message[MAX_PIPE_NAME + U_INT8_T];

    u_int8_t code = CODE_LIST_BOXES;
    memcpy(message, &code, U_INT8_T);
    memcpy(message+U_INT8_T, ppipe_name, MAX_PIPE_NAME);

    ssize_t wr = write(rp, message, message_size); // sends request to mbroker
    close(rp); // close mbroker register pipe

    int mp = open(manager_pipe,O_RDONLY); // open pipe for receiving communications from mbroker 
    if (mp == -1) {
        fprintf(stdout, "Error opening pipe\n");
        return;
    }
    if (wr != message_size) {
        fprintf(stdout, "Error communicating with mbroker.\n");
        return;
    }

    u_int64_t b_size, n_sub, n_pub;
    u_int8_t last;

    size_t i=0, array_size = 64;
    box_info *array;
    array = (box_info*) malloc(sizeof(box_info)*64);
    while (true) {
        char name[MAX_BOX_NAME];
        if (read(mp, &code, U_INT8_T) != U_INT8_T || read(mp, &last, U_INT8_T) != U_INT8_T 
            || read(mp, name, MAX_BOX_NAME) != MAX_BOX_NAME || 
            read(mp, &b_size, U_INT64_T) != U_INT64_T || read(mp, &n_pub, U_INT64_T) != U_INT64_T 
            || read(mp, &n_sub, U_INT64_T) != U_INT64_T) {
            fprintf(stdout, "Error communicating with mbroker.\n");
            return;
        }
        ALWAYS_ASSERT(code == CODE_LIST_BOXES+1, "Error while managing boxes.\n");
        if (last && name[0] == 0) { // means there's no boxes in mbroker
            fprintf(stdout, "NO BOXES FOUND\n");
            return;
        }
        if (i == array_size) { // currently allocated array has been filled
            array_size *= 2;
            array = (box_info*) realloc(array, array_size); // reallocates new array with double the size
        }
        strcpy(array[i].box_name, name);
        array[i].size = b_size;
        array[i].n_publishers = n_pub;
        array[i].n_subscribers = n_sub;

        if (last) // if the sent box is the last one in mbroker
            break;
        i++;
    }

    qsort(array, i, sizeof(box_info), cmpbox); // sorts boxes in alphabetical order

    for (int j = 0; j <= i; j++)
        fprintf(stdout, "%s %zu %zu %zu\n", 
            array[j].box_name, array[j].size, array[j].n_publishers, array[j].n_subscribers);

    if (unlink(manager_pipe) != 0) {
        fprintf(stderr, "Error: unlink(%s) failed: %s\n", manager_pipe,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    if (argc != 4 && argc != 5) {
        fprintf(stdout, "Unexpected number of arguments\n");
        exit(0);
    }
    register_pipe = argv[1];
    manager_pipe = argv[2];
    if (unlink(manager_pipe) != 0 && errno != ENOENT) {
        fprintf(stderr, "Error: unlink(%s) failed: %s\n", manager_pipe,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (mkfifo(manager_pipe, 0640) != 0) { // create pipe for communication wit mbroker
        fprintf(stderr, "Error: mkfifo failed: %s\n", strerror(errno));
        return -1;
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) { // the signal SIGPIPE is ignored
        exit(EXIT_FAILURE);
    }

    if (!strcmp(argv[3], "create")) // create a new box
        create(argv[4]);

    else if (!strcmp(argv[3], "remove")) // remove a box
        delete(argv[4]);
    
    else if (!strcmp(argv[3], "list")) // list all boxes
        list_boxes();

    else {
        fprintf(stdout, "Unknown manager command.\n");
        return -1;
    }
    return 0;
}
