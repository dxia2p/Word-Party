#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

// structure so we can pass multiple arguments to thread
struct message_queue_group {
    struct message_queue *read_q;
    struct message_queue *write_q;
};

// Messages in this queue use the same protocol as network messages
struct message_queue;
struct message_queue *create_message_queue();
void destroy_message_queue(struct message_queue *mq);
int get_read_pipe(struct message_queue *mq);
void message_enqueue(struct message_queue *mq, char *msg, int msg_len);
int message_dequeue(struct message_queue *mq, char *dest, int dest_max_len);


/*
struct queue;
struct queue *create_queue();
void destroy_queue(struct queue *q);
void enqueue(struct queue *q, char *msg, int msg_len);
void dequeue(struct queue *q, char *msg, int msg_max_size);
int get_queue_size(struct queue *q);
*/

#endif
