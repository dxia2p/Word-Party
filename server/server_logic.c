#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message_queue.h"
#include "../public/custom_protocol.h"
#include "thread_protocol_codes.h"

const int MAX_HP = 2;
struct player_data {
    int id;
    char name[26];
    int hp;
    struct player_data *next;
};

// Function prototypes
static void process_thread_command(char *msg);
static void create_player_data(int id, char *name);
void delete_player_data(int id);

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
        process_thread_command(mq_buf);
        message_size = message_dequeue(read_queue, mq_buf, sizeof(mq_buf));
    }
}

struct player_data *get_player_by_id(int id) {
    struct player_data *cur = players;
    while (cur != NULL && cur->id != id) {
        cur = cur->next;
    }
    
    if (cur == NULL) {  // This only happens if given id does not belong to any player
        fprintf(stderr, "Invalid id (%d) in get_player_by_id()\n", id);
        return NULL;
    }

    return cur;
}

void create_player_data(int id, char *name) {
    struct player_data *temp = malloc(sizeof(struct player_data));
    temp->id = id;
    strcpy(temp->name, name);
    temp->hp = MAX_HP;
    temp->next = players;
    players = temp;
}

void delete_player_data(int id) {

}

static void process_thread_command(char *msg) {
    printf("from thread: %s\n", msg);
    char code = get_msg_code(msg);
    int len = get_msg_len(msg);
    static char temp_buf[256];

    switch(code){
        case T_PLAYER_JOIN: {
            int id;
            parse_msg_body(msg, thread_protocol_fmtstrs, sizeof(thread_protocol_fmtstrs), &id, temp_buf);      
            create_player_data(id, temp_buf);
            printf("Created player data (id: %d) (name: %s) in logic thread!\n", id, temp_buf);
            break;
        }
        case T_SEND_NAME: {
            int id;
            parse_msg_body(msg, thread_protocol_fmtstrs, sizeof(thread_protocol_fmtstrs), &id, temp_buf);
            strcpy(get_player_by_id(id)->name, temp_buf);
            printf("Player id (%d) changed their name to %s\n", id, temp_buf);
        }
        default:
            // Do something
            break;
    }
}
