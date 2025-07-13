#include <stdio.h>
#include <stdlib.h>

#include "message_queue.h"
#include "../public/custom_protocol.h"
#include "thread_protocol_codes.h"

struct player_data {
    int id;
    char name[26];
    int hp;
    struct player_data *next;
};

// Function prototypes
static void process_thread_command(char *msg);

// variables
struct player_data *players = NULL;
static struct message_queue *write_queue;
static struct message_queue *read_queue;

void game_init(struct message_queue_group *queue_group) {
    write_queue = queue_group->write_q;
    read_queue = queue_group->read_q;
}

void game_update() {
    char mq_buf[512];
    int message_size = message_dequeue(read_queue, mq_buf, sizeof(mq_buf));
    while (message_size != 0) {
        message_size = message_dequeue(read_queue, mq_buf, sizeof(mq_buf));
        process_thread_command(mq_buf);
    }
}

void create_player_data(int id) {
    
}

static void process_thread_command(char *msg) {
    char code = get_msg_code(msg);
    int len = get_msg_len(msg);
    //static char temp_buf[512];

    switch(code){
        default:
            message_enqueue(write_queue, msg, len);
            break;
    }
}
