#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "logging.h"
#include "mbroker.h"
#include "../fs/operations.h"
#include "../fs/state.h"
#include "../utils/betterassert.h"
#include "../producer-consumer/producer-consumer.h"

int rp; // register pipe file descriptor
char *register_pipe_name; // name of the register pipe
size_t max_sessions; // maximum amount of sessions open simultaneously
box_list list = NULL; // list where boxes are stored
pc_queue_t *queue; // producer-consumer queue


int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc == 3, "Wrong number of arguments passed.");
    ALWAYS_ASSERT(tfs_init(NULL) != -1, "Error starting TFS");
    register_pipe_name = argv[1];
    if (unlink(register_pipe_name) != 0 && errno != ENOENT) { // problem deleting previously created pipe
        fprintf(stderr, "Error: unlink(%s) failed: %s\n", register_pipe_name,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (mkfifo(register_pipe_name, 0640) != 0) // create register pipe
        return -1;
    max_sessions = (size_t) atoi(argv[2]);

    if (signal(SIGINT, sig_handler) == SIG_ERR || 
        signal(SIGQUIT, SIG_IGN) == SIG_ERR || 
        signal(SIGTERM, SIG_IGN) == SIG_ERR ||
        signal(SIGPIPE, SIG_IGN) == SIG_ERR ||
        signal(SIGUSR1, sig_handler) == SIG_ERR) {
            exit(EXIT_FAILURE);
    }

    queue = (pc_queue_t*) malloc(sizeof(pc_queue_t));
    void **buffer = (void**) malloc(sizeof(void*)*max_sessions*2);
    queue->pcq_buffer = buffer;
    pcq_create(queue, 2*max_sessions); // initialize producer-consumer queue

    pthread_t *workers = (pthread_t*) malloc(sizeof(pthread_t)*max_sessions);
    for (int i = 0; i < max_sessions; i++) {
        pthread_create(&workers[i], NULL, wait_for_command, NULL); // initialize worker threads
    }

    rp = open(register_pipe_name, O_RDONLY); // open register pipe
    int rpw = open(register_pipe_name, O_WRONLY); // open for writing to avoid active standby 
    if (rp == -1 || rpw == -1)
        return -1;
    while (true) { // waits for input in register pipe, until stopped
        u_int8_t code;
        ssize_t ret = read(rp, &code, 1);
        if (ret) {
            ALWAYS_ASSERT(ret != -1, "Error reading register pipe\n");
            if (put_in_queue(code) == -1) {
                fprintf(stdout, "Error putting arguments in queue\n");
                if (unlink(register_pipe_name) != 0) {
                    fprintf(stderr, "Error: unlink(%s) failed: %s\n", register_pipe_name,
                        strerror(errno));
                }
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}


/*
 * This functions adds a newly created box to the server (in alphabetical order).

 * Returns -1 if there's an error with the file system (like being full),
 * false (0) if there's already a box with the given name, and returns true (1)
 * if the box was added successfully.
 *
 */
int add_to_box_list(box new_box) {
    box_list new_entry = (box_list) malloc(sizeof(struct b_list));
    new_entry->tbox = new_box;
    new_entry->next = NULL;
    if (list == NULL) { // no boxes in system
        list = new_entry;
    }
    else {
        box_list aux = list, last = NULL;
        int cmp;
        while (aux != NULL) {
            cmp = strcmp(new_box.name,aux->tbox.name);
            if (!cmp) { // has same name as box already in system
                free(new_entry);
                return false;
            }
            if (cmp < 0) { // has found it's place
                if (last == NULL)
                    list = new_entry;
                else
                    last->next = new_entry;
                new_entry->next = aux;
                break;
            }
            last = aux;
            aux = aux->next;
        }
        if (aux == NULL && last != NULL) // its placed in the end of list
            last->next = new_entry;
    }
    if (tfs_create_file(new_box.name) == -1) {
        free(new_entry);
        return -1;
    }
    return true;
}


/*
 * This functions removes a box from the server.
 *
 * Returns -1 if there's an error with the file system,
 * false (0) if there's no box with the given name, and returns true (1)
 * if the box was removed successfully.
 *
 */
int remove_from_box_list(char *box_name) {
    box_list aux = list, past = NULL;
    while (aux != NULL) { // iterates through the box list
        if (!strcmp(aux->tbox.name,box_name)) { // if the given name matches with one on the system
            if (past == NULL)
                list = aux->next;
            else {
                past->next = aux->next;
            }
            char *file_name = get_file_name(aux->tbox.name);
            if (tfs_unlink(file_name) == -1) { // deleting the boxe's file
                free(file_name);
                return -1;
            }
            free(file_name);
            aux->tbox.deleted = true; // to warn subscriber threads of removal
            pthread_cond_broadcast(&aux->tbox.message_condvar); // wakes subscriber threads
            if (aux->tbox.publisher != 0)
                pthread_kill(aux->tbox.publisher, SIGUSR1); // sends signal to publisher thread which ends it
            return true;
        }
        past = aux;
        aux = aux->next;
    }
    return false; // there's no box with the given name
}

/**
 * Associates a publisher to a box.
 * 
 * If the box exists in the server, returns it's entry.
 * Otherwise, return NULL.
 * 
 */
box_list assoc_pub(char *box_name) {
    for (box_list aux = list; aux != NULL; aux = aux->next) {
        if (!strcmp(aux->tbox.name, box_name)) {
            if (aux->tbox.has_publisher)
                return NULL;
            aux->tbox.has_publisher = true; // marks box as occupied by publisher
            aux->tbox.publisher = pthread_self(); // stores the publisher's thread 
            return aux;
        }
    }
    return NULL;
}

/**
 * Associates a subscriber with a box.
 * 
 * If the box exists in the server, returns it's entry.
 * Otherwise, return NULL.
 * 
 */
box_list assoc_sub(char *box_name) {
    for (box_list entry = list; entry != NULL; entry = entry->next) {
        if (!strcmp(entry->tbox.name, box_name)) {
            entry->tbox.n_subscribers++;
            return entry;
        }
    }
    return NULL;
}

/**
 * Creates a "file name", which consists of the box's name with a '/' placed in
 * the beginning.
 * 
 * Returns the file name.
 * 
 */
char *get_file_name(char *box_name) {
    char *file_name, *aux;
    file_name = (char*) malloc(sizeof(char)*(MAX_BOX_NAME+1));
    aux = file_name;
    memset(aux, '/', 1);
    aux++;
    memcpy(aux, box_name, MAX_BOX_NAME);
    return file_name;
}


/**
 * This function reads the arguments passed from the register pipe and 
 * puts them in the producer-consumer queue for processing.
 * 
 */
int put_in_queue(u_int8_t code) {
    switch (code) { // determines the command by the given code

        case REGISTER_PUB: { // code used to register a new publisher to a box
            char client_pipe[MAX_PIPE_NAME], box_name[MAX_BOX_NAME];
            if (read(rp, client_pipe, MAX_PIPE_NAME) != MAX_PIPE_NAME 
                || read(rp, box_name, MAX_BOX_NAME) != MAX_BOX_NAME)
                    return -1;

            command_args *args = (command_args*) malloc(sizeof(command_args));
            args->code = REGISTER_PUB;
            strcpy(args->client_pipe, client_pipe);
            strcpy(args->box_name, box_name);

            // puts arguments in producer-consumer queue
            ALWAYS_ASSERT(pcq_enqueue(queue, args) != -1, "Error while accessing request queue\n");
        } break;


        case REGISTER_SUB: { // code used to register a new subscriber to a box
            char client_pipe[MAX_PIPE_NAME], box_name[MAX_BOX_NAME];
            if (read(rp, client_pipe, MAX_PIPE_NAME) != MAX_PIPE_NAME 
                || read(rp, box_name, MAX_BOX_NAME) != MAX_BOX_NAME)
                    return -1;

            command_args *args = (command_args*) malloc(sizeof(command_args));
            args->code = REGISTER_SUB;
            strcpy(args->client_pipe, client_pipe);
            strcpy(args->box_name, box_name);

            // puts arguments in producer-consumer queue
            ALWAYS_ASSERT(pcq_enqueue(queue, args) != -1, "Error while accessing request queue\n");
        } break;


        case REGISTER_BOX: { // code used for adding a new box to the server
            char client_pipe[MAX_PIPE_NAME], box_name[MAX_BOX_NAME];

            if (read(rp, client_pipe, MAX_PIPE_NAME) != MAX_PIPE_NAME 
                || read(rp, box_name, MAX_BOX_NAME) != MAX_BOX_NAME)
                return -1;
            
            command_args *args = (command_args*) malloc(sizeof(command_args));
            args->code = REGISTER_BOX;
            strcpy(args->client_pipe, client_pipe);
            strcpy(args->box_name, box_name);

            // puts arguments in producer-consumer queue
            ALWAYS_ASSERT(pcq_enqueue(queue, args) != -1, "Error while accessing request queue\n");
        } break;


        case REMOVE_BOX: { // code used to remove a box from the server
            char client_pipe[MAX_PIPE_NAME], box_name[MAX_BOX_NAME];
            if (read(rp, client_pipe, MAX_PIPE_NAME) != MAX_PIPE_NAME 
                || read(rp, box_name, MAX_BOX_NAME) != MAX_BOX_NAME)
                return -1;

            command_args *args = (command_args*) malloc(sizeof(command_args));
            args->code = REMOVE_BOX;
            strcpy(args->client_pipe, client_pipe);
            strcpy(args->box_name, box_name);

            // puts arguments in producer-consumer queue
            ALWAYS_ASSERT(pcq_enqueue(queue, args) != -1, "Error while accessing request queue\n");        
        } break;


        case LIST_BOXES: { // code used to list all the boxes from the server
            char client_pipe[MAX_PIPE_NAME];
            if (read(rp, client_pipe, MAX_PIPE_NAME) != MAX_PIPE_NAME)
                return -1;

            command_args *args = (command_args*) malloc(sizeof(command_args));
            args->code = LIST_BOXES;
            strcpy(args->client_pipe,client_pipe);

            // puts arguments in producer-consumer queue
            ALWAYS_ASSERT(pcq_enqueue(queue, args) != -1, "Error while accessing request queue\n");
        } break;
        default:
    }
    return 0;
}

/**
 * This is the function run by the threads.
 * 
 * Waits for arguments to be put in queue and determines which action to take
 * according to the given code.
 * 
 */
void *wait_for_command() {
    while (true) {
        // gets arguments from queue
        // if there aren't any, waits (no active standby)
        command_args *arguments = (command_args*) pcq_dequeue(queue);
        ALWAYS_ASSERT(arguments != NULL, "Error while accessing request queue\n");
        switch(arguments->code) {

            case REGISTER_PUB: {
                register_publisher(arguments);
            } break;

            case REGISTER_SUB: {
                register_subscriber(arguments);
            } break;

            case REGISTER_BOX: {
                register_box(arguments);
            } break;

            case REMOVE_BOX: {
                remove_box(arguments);
            } break;

            case LIST_BOXES: {
                list_boxes(arguments);
            } break;
            default:
        }
        free(arguments);
    }
    return NULL;
}

/**
 * Sends information about every box in server, to be listed.
 * 
 */
void list_boxes(command_args *arguments) {
    int cp = open(arguments->client_pipe, O_WRONLY); // opens the manager pipe for writing
    size_t message_size = U_INT8_T*2 + MAX_BOX_NAME + U_INT64_T*3;
    char name[MAX_BOX_NAME];
    char message[U_INT8_T*2 + MAX_BOX_NAME + U_INT64_T*3], *aux;
    u_int8_t code_send = CODE_LIST_BOXES;

    if (list == NULL) { // there are no boxes in the server
        aux = message;
        memset(message, 0, message_size);

        u_int8_t last = LAST;
        memcpy(aux, &code_send, U_INT8_T);
        aux++;
        memcpy(aux, &last, U_INT8_T);
            
        if (write(cp, message, message_size) != message_size) {
            close(cp);
            return;
        }
        close(cp);
        return;
    }

    for (box_list entry = list; entry != NULL; entry = entry->next) {
        aux = message;
        memset(name, 0, MAX_BOX_NAME);
        strcpy(name, entry->tbox.name);

        char *file_name = get_file_name(name);
        u_int64_t size = tfs_get_file_size(file_name), n_pub = entry->tbox.has_publisher,
            n_sub = entry->tbox.n_subscribers;
        free(file_name);
        u_int8_t last = 0;
        if (entry->next == NULL) // this entry is the last one in server
            last = 1;

        memcpy(aux, &code_send, U_INT8_T); // puts code in message
        aux++;
        memcpy(aux, &last, U_INT8_T); // puts 'last' in message (1 if last entry, 0 otherwise)
        aux++;
        memcpy(aux, name, MAX_BOX_NAME); // puts the entry's name in message
        aux += MAX_BOX_NAME;
        memcpy(aux, &size, U_INT64_T); // puts the entry's file size in message
        aux += U_INT64_T;
        memcpy(aux, &n_pub, U_INT64_T); // puts the number of publishers in the box in message
        aux += U_INT64_T;
        memcpy(aux, &n_sub, U_INT64_T); // puts the number of subscribers in the box in message

        if (write(cp, message, message_size) != message_size)
            break;
    }
    close(cp);
}

/**
 * Registers a new box in the system, with the given arguments.
 * 
 * Then reports back to the manager with information about the request 
 * (if successful / what errors may have occurred).
 * 
 */
void register_box(command_args *arguments) {
    size_t message_size = U_INT8_T + INT32_T + MAX_ERROR_MSG;
    int cp;
    char message[U_INT8_T + INT32_T + MAX_ERROR_MSG], *aux = message;
    char error[MAX_ERROR_MSG];

    box new;
    new.has_publisher = false; // starts with no publisher associated
    new.n_subscribers = 0; // starts with no subscribers associated
    new.deleted = false;
    new.publisher = 0;
    pthread_mutex_init(&new.message_lock, NULL);
    pthread_cond_init(&new.message_condvar, NULL);
    strcpy(new.name, arguments->box_name);

    cp = open(arguments->client_pipe, O_WRONLY); // opens the manager pipe for writing 
    int add = add_to_box_list(new); // adds the new box to the list
    memset(error, 0, MAX_ERROR_MSG);

    u_int8_t code_send = CODE_ADD_BOX;
    int32_t return_code;
    memcpy(aux, &code_send, U_INT8_T);
    aux++;

    switch (add) {
        case true: { // box added successfully
            return_code = RETURN_0;
            memcpy(aux, &return_code, INT32_T);
        } break;

        case false: { // box name already taken
            strcpy(error, "Box name already taken.\n");
            return_code = RETURN_ERROR;
            memcpy(aux, &return_code, INT32_T);
        } break;

        case -1: { // error in file system
            strcpy(error, "Error creating box file.\n");
            return_code = RETURN_ERROR;
            memcpy(aux, &return_code, INT32_T);
        } break;

        default:
    }

    aux += INT32_T;
    memcpy(aux, error, MAX_ERROR_MSG);
    if (write(cp, message, message_size) != message_size) {
        close(cp);
        return;
    }
    close(cp);
}

/**
 * Removes a box from the system, with the given arguments.
 * 
 * Then reports back to the manager with information about the request 
 * (if successful / what errors may have occurred).
 * 
 */
void remove_box(command_args *arguments) {
    size_t message_size = U_INT8_T + INT32_T + MAX_ERROR_MSG;
    int cp = open(arguments->client_pipe, O_WRONLY); // opens the manager's pipe
    int rm = remove_from_box_list(arguments->box_name); // removes the box from list
            
    char message[U_INT8_T + INT32_T + MAX_ERROR_MSG], *aux = message;
    char error[MAX_ERROR_MSG];
    memset(error, 0, MAX_ERROR_MSG);

    u_int8_t code_send = CODE_RM_BOX;
    int32_t return_code;
    memcpy(aux, &code_send, U_INT8_T);
    aux++;

    switch (rm) {
        case true: { // box removed successfully
            return_code = RETURN_0;
            memcpy(aux, &return_code, INT32_T);
        } break;

        case false: { // no box with given name found
            strcpy(error, "Box not found in system.\n");
            return_code = RETURN_ERROR;
            memcpy(aux, &return_code, INT32_T); 
        } break;

        case -1: { // error in file system
            strcpy(error, "Error deleting box file.\n");
            return_code = RETURN_ERROR;
            memcpy(aux, &return_code, INT32_T);
        } break;

        default:
    }

    aux += INT32_T;
    memcpy(aux, error, MAX_ERROR_MSG);
    if (write(cp, message, message_size) != message_size) {
        close(cp);
        return;
    }
    close(cp);
}

/**
 * Registers a new publisher and waits for new messages to put in it's box.
 * 
 * Only returns when the box is deleted or the publisher disconnects.
 * 
 */
void register_publisher(command_args *arguments){
    int cp = open(arguments->client_pipe, O_RDONLY); // opens the publisher's pipe
    box_list entry = assoc_pub(arguments->box_name); // associates the publisher with given box
    if (entry == NULL) { // box either is occupied by other publisher or doesn't exist
        close(cp);
        return;
    }

    char *file_name = get_file_name(arguments->box_name);
    int fd = tfs_open(file_name, TFS_O_APPEND); // opens the box's file
    free(file_name);
    if (fd == -1) {
        entry->tbox.has_publisher = false;
        close(cp);
    }

    u_int8_t code;
    char buffer[MAX_PUB_SIZE];
    while (read(cp, &code, U_INT8_T) > 0) { // while the read doesn't fail
        if (pthread_mutex_lock(&entry->tbox.message_lock) == -1) {
            break;
        }
        if (code != CODE_RECEIVE_PUB) {
            pthread_mutex_unlock(&entry->tbox.message_lock);
            break;
        }
        if (read(cp, buffer, MAX_PUB_SIZE) != MAX_PUB_SIZE) {
            pthread_mutex_unlock(&entry->tbox.message_lock);
            break;
        }
        size_t pub_size = strlen(buffer);
        buffer[pub_size-1] = '\0';
        char message[MAX_PUB_SIZE];
        strcpy(message, buffer);

        // if the file is full or the write is unsuccessful, it will fail and close session
        if (tfs_write(fd, message, pub_size) <= 0) {
            pthread_mutex_unlock(&entry->tbox.message_lock);
            break;
        }

        memset(buffer, 0, MAX_PUB_SIZE);
        pthread_cond_broadcast(&entry->tbox.message_condvar); // wakes subscriber threads who were waiting for new message
        pthread_mutex_unlock(&entry->tbox.message_lock);
    }

    // disassociates publisher from box
    entry->tbox.has_publisher = false;
    entry->tbox.has_publisher = 0;

    // closes open file descriptors
    tfs_close(fd);
    close(cp);
}

/**
 * Registers a new subscriber and waits for new messages to return them to the
 * subscriber.
 * 
 * Only returns when the box is deleted or the subscriber disconnects.
 * 
 */
void register_subscriber(command_args *arguments) {
    int cp = open(arguments->client_pipe, O_WRONLY); // opens the subscriber's pipe

    char *file_name = get_file_name(arguments->box_name);
    int fd = tfs_open(file_name, TFS_O_CREAT); // opens the box's file
    free(file_name);
    if (fd == -1) {
        close(cp);
    }

    box_list entry = assoc_sub(arguments->box_name); // associates the subscriber with given box
    
    if (entry == NULL) { // box isn't in system
        tfs_close(fd);
        close(cp);
        return;
    }

    char buffer[MAX_PUB_SIZE];
    if (pthread_mutex_lock(&entry->tbox.message_lock) == -1) {
        tfs_close(fd);
        close(cp);
        return;
    }

    ssize_t read = tfs_read(fd, buffer, MAX_PUB_SIZE); // reads box's file
    u_int8_t code_send = CODE_SEND_PUB;
    while (true) {
        while (read == 0) {
            // if it has read everything in the box, waits for new publication
            pthread_cond_wait(&entry->tbox.message_condvar, &entry->tbox.message_lock);
            if (entry->tbox.deleted == true) { // if it has been woke because box has been deleted
                pthread_mutex_unlock(&entry->tbox.message_lock);
                tfs_close(fd);
                close(cp);
            }
            read = tfs_read(fd, buffer, MAX_PUB_SIZE); // reads box's file
        }
        if (read == -1) // failed reading file
            break;
        size_t incr = 0;
        char message[MAX_PUB_SIZE];
        while (incr < read) { // while all the messages haven's been sent
            memset(message, 0, MAX_PUB_SIZE);
            strcpy(message, buffer + incr);
            incr += strlen(message) + 1; // next message's starting point

            if (write(cp, &code_send, U_INT8_T) != U_INT8_T || 
                write(cp, message, MAX_PUB_SIZE) != MAX_PUB_SIZE) {
                entry->tbox.n_subscribers--;
                tfs_close(fd);
                close(cp);
                return;
            }
        }
        memset(buffer, 0, MAX_PUB_SIZE);
        read = tfs_read(fd, buffer, MAX_PUB_SIZE); // reads box's file
    }

    pthread_mutex_unlock(&entry->tbox.message_lock);

    entry->tbox.n_subscribers--;
    
    // closes open file descriptors
    tfs_close(fd);
    close(cp);
}

/**
 * Handles the signal SIGINT (Ctrl-C) to exit correctly.
 * 
 */
static void sig_handler(int sig) {
    if (sig == SIGINT) {
        if (unlink(register_pipe_name) != 0) {
            fprintf(stderr, "Error: unlink(%s) failed: %s\n", register_pipe_name,
                strerror(errno));
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
}