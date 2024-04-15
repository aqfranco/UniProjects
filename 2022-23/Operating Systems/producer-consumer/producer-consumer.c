#include <pthread.h>
#include "producer-consumer.h"
#include <string.h>

int pcq_create(pc_queue_t *queue, size_t capacity){
    if(queue->pcq_buffer == NULL){
        return -1;
    }
    pthread_mutex_init(&queue->pcq_current_size_lock, NULL);
    pthread_mutex_init(&queue->pcq_head_lock, NULL);
    pthread_mutex_init(&queue->pcq_tail_lock, NULL);
    pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL);
    pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL);
    pthread_cond_init(&queue->pcq_pusher_condvar, NULL);
    pthread_cond_init(&queue->pcq_popper_condvar, NULL);
    queue->pcq_capacity = capacity;
    queue->pcq_current_size = 0;
    queue->pcq_head = 0;
    queue->pcq_tail = 0;
    return 0;
}

int pcq_destroy(pc_queue_t *queue){
    if(pthread_mutex_lock(&queue->pcq_current_size_lock) != 0){
        return -1;
    }
    queue->pcq_current_size = 0;
    pthread_mutex_unlock(&queue->pcq_current_size_lock);
    pthread_mutex_destroy(&queue->pcq_current_size_lock);
    pthread_mutex_destroy(&queue->pcq_head_lock);
    pthread_mutex_destroy(&queue->pcq_tail_lock);
    pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock);
    pthread_mutex_destroy(&queue->pcq_popper_condvar_lock);
    pthread_cond_destroy(&queue->pcq_pusher_condvar);
    pthread_cond_destroy(&queue->pcq_popper_condvar);
    return 0;
}

int pcq_enqueue(pc_queue_t *queue, void *elem){
    if(pthread_mutex_lock(&queue->pcq_pusher_condvar_lock) != 0){
        return -1;
    }
    while(queue->pcq_current_size == queue->pcq_capacity){
        pthread_cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_pusher_condvar_lock);
    }
    if(pthread_mutex_lock(&queue->pcq_head_lock) != 0){
        return -1;
    }
    queue->pcq_buffer[queue->pcq_head] = elem;
    if (queue->pcq_head < queue->pcq_capacity)
        queue->pcq_head++;
    else 
        queue->pcq_head = 0;
    if(pthread_mutex_unlock(&queue->pcq_head_lock) != 0){
        return -1;
    }
    if(pthread_mutex_lock(&queue->pcq_current_size_lock) != 0){
        return -1;
    }
    queue->pcq_current_size++;
    pthread_cond_signal(&queue->pcq_popper_condvar);
    pthread_mutex_unlock(&queue->pcq_current_size_lock);
    pthread_mutex_unlock(&queue->pcq_pusher_condvar_lock);
    return 0;
}

void *pcq_dequeue(pc_queue_t *queue){
    if(pthread_mutex_lock(&queue->pcq_popper_condvar_lock) != 0){
        return NULL;
    }
    while(queue->pcq_current_size == 0){
        pthread_cond_wait(&queue->pcq_popper_condvar, &queue->pcq_popper_condvar_lock);
    }
    if(pthread_mutex_lock(&queue->pcq_tail_lock) != 0){
        return NULL;
    }
    void *args = queue->pcq_buffer[queue->pcq_tail];
    if (queue->pcq_tail < queue->pcq_capacity)
        queue->pcq_tail++;
    else 
        queue->pcq_tail = 0;
    if(pthread_mutex_unlock(&queue->pcq_tail_lock) != 0){
        return NULL;
    }
    if(pthread_mutex_lock(&queue->pcq_current_size_lock) != 0){
        return NULL;
    }
    queue->pcq_current_size--;
    pthread_cond_signal(&queue->pcq_pusher_condvar);
    pthread_mutex_unlock(&queue->pcq_current_size_lock);
    pthread_mutex_unlock(&queue->pcq_popper_condvar_lock);
    return args;
}

