#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <unistd.h>
#include <stdbool.h>
#include "../public/network_header.h"
#include "message_queue.h"

#define MAX_MESSAGE_QUEUE_ITEM_SIZE 256
#define MESSAGE_QUEUE_LEN 20



// Single producer single consumer queue (thread safe)
struct message_queue {
    char buffer[MESSAGE_QUEUE_LEN][MAX_MESSAGE_QUEUE_ITEM_SIZE];
    int buffer_lengths[MESSAGE_QUEUE_LEN];

    _Atomic int head;
    _Atomic int tail;

    int pipe[2];
};

struct message_queue *create_message_queue() {
    struct message_queue *temp = calloc(1, sizeof(struct message_queue));
    pipe(temp->pipe);
    return temp;
}

void destroy_message_queue(struct message_queue *mq) {
    free(mq);
}

// For polling applications
/*
bool is_message_queue_readable(struct message_queue *mq) {
    fd_set readfds;
    struct timeval timeout = {0, 0};

    FD_ZERO(&readfds);
    FD_SET(mq->pipe[0], &readfds);

    if (select(mq->pipe[0] + 1, &readfds, NULL, NULL, &timeout) == -1) {
        fprintf(stderr, "select() failed in is_message_queue_readable() (%d)\n", GETSOCKETERRNO());
    }
    return FD_ISSET(mq->pipe[0], &readfds);
}
*/
int get_read_pipe(struct message_queue *mq) {
    return mq->pipe[0];
}

void message_enqueue(struct message_queue *mq, char *msg, int msg_len) {  // enqueue from tail, dequeue from head
    int tail = atomic_load(&mq->tail);
    int head = atomic_load(&mq->head);
    if (msg_len > MAX_MESSAGE_QUEUE_ITEM_SIZE) {
        fprintf(stderr, "message length is too large for message queue\n");
        return;
    }

    if ((tail + 1) % MESSAGE_QUEUE_LEN != head) {
        strncpy(mq->buffer[tail], msg, msg_len);
        mq->buffer_lengths[tail] = msg_len;
        atomic_store(&mq->tail, (tail + 1) % MESSAGE_QUEUE_LEN);
        write(mq->pipe[1], "x", 1);
    } else {
        fprintf(stderr, "Not enough space in message queue\n");
    }
}

// Returns size of message dequeued
int message_dequeue(struct message_queue *mq, char *dest, int dest_max_len) {
    int head = atomic_load(&mq->head);
    int tail = atomic_load(&mq->tail);

    if (head != tail) {
        int buf_len = mq->buffer_lengths[head];
        if (buf_len > dest_max_len) {
            fprintf(stderr, "destination length is insufficient in message_dequeue()\n");
            return -1;
        }
        strncpy(dest, mq->buffer[head], buf_len);
        atomic_store(&mq->head, (head + 1) % MESSAGE_QUEUE_LEN);
        char buf[1];
        read(mq->pipe[0], buf, 1);
        return buf_len;
    }
    return 0;
}

// ------------------------------------------------------- Linked list queue ------------------------------------------------------------

/*

struct message_queue {
    struct queue *q;
    int pipe[2];
};

void message_enqueue(struct message_queue* mq, char *msg, int msg_len) {
    enqueue(mq->q, msg, msg_len);
}


void message_dequeue() {

}
*/

/*
struct queue_node {   // head -> qn -> qn -> ... -> qn -> tail
    char *msg;
    int msg_size;
    struct queue_node *next;
};

struct queue {  // Enqueue from tail, dequeue from head
    struct queue_node *head;
    struct queue_node *tail;
    int size;
};

struct queue *create_queue() {
    return calloc(1, sizeof(struct queue));
}

void destroy_queue(struct queue *q) {
    struct queue_node *cur = q->head;
    while (cur != NULL) {
        free(cur->msg);
        struct queue_node *temp = cur->next;
        free(cur);
        cur = temp;
    }
    free(q);
}

void enqueue(struct queue *q, char *msg, int msg_len) {
    struct queue_node *temp = malloc(1 * sizeof(struct queue_node));
    temp->msg = strdup(msg);
    temp->msg_size = msg_len;
    temp->next = NULL;
    if (q->size == 0) {
        q->tail = temp;
        q->head = temp;
    } else {
        q->tail->next = temp;
        q->tail = temp;
    }
    q->size++;
}

void dequeue(struct queue *q, char *msg, int msg_max_size) {
    if (q->size == 0) {
        fprintf(stderr, "nothing to dequeue()\n");
        return;
    }
    struct queue_node *temp = q->head;
    if (q->size == 1) {
        q->tail = NULL;
    }
    q->head = q->head->next;
    if (temp->msg_size > msg_max_size) {
        fprintf(stderr, "msg_max_size in dequeue() is insufficient (required size: %d)\n", temp->msg_size);
        return;
    }
    strncpy(msg, temp->msg, temp->msg_size);
    free(temp->msg);
    free(temp);
    q->size--;
}

int get_queue_size(struct queue *q) {
    return q->size;
}
*/
