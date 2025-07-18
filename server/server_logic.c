#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "message_queue.h"
#include "../public/custom_protocol.h"
#include "../public/network_header.h"
#include "thread_protocol_codes.h"
#include "word_validator.h"

/* Constants */
const int MAX_HP = 2;
const double MAX_TURN_TIMER = 10.0;
const double MAX_SEND_TIMER = 0.5;
const char ALPHABET[] = "abcdefghijklmnopqrstuvwxyz";
const int ALPHABET_LEN = 26;

struct player_data {
    int id;
    int hp;
    struct player_data *next;
};

/* Function prototypes */
static void game_start();
static void next_turn();
static struct player_data *get_player_by_id(int id);
static void create_player_data(int id);
static void delete_player_data(int id);
static void process_thread_command(char *msg);
static void process_terminal_command(char *cmd);

/* variables */
struct player_data *players = NULL;
static struct message_queue *write_queue;
static struct message_queue *read_queue;
static bool game_started = false;
static struct player_data *player_turn;
static char required_substr[4];  // A players word needs to contain this substring
static char word_substrs[8500][4];
static int word_substrs_len;

/* Functions */

// Gets called once at the beginning
void game_init(struct message_queue_group *queue_group) {
    write_queue = queue_group->write_q;
    read_queue = queue_group->read_q;
    srand(time(NULL));

    initialize_substr_list("../server/assets/substr_list.txt");  // Change this to /assets/substr_list.txt when releasing game
}

// Gets called every frame
void game_update(double deltatime) {
    static char mq_buf[512];  // Message queue buffer

    struct timeval tv;
    fd_set stdin_fd_set;
    FD_ZERO(&stdin_fd_set);
    FD_SET(STDIN_FILENO, &stdin_fd_set);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    select(STDIN_FILENO + 1, &stdin_fd_set, NULL, NULL, &tv);
    if (FD_ISSET(STDIN_FILENO, &stdin_fd_set)){
        static char stdin_buf[256];
        if (fgets(stdin_buf, sizeof(stdin_buf), stdin) != NULL) {
            stdin_buf[strlen(stdin_buf) - 1] = '\0';  // Trim newline
            process_terminal_command(stdin_buf);
        }
    }
    
    int message_size = message_dequeue(read_queue, mq_buf, sizeof(mq_buf));
    while (message_size != 0) {
        process_thread_command(mq_buf);
        message_size = message_dequeue(read_queue, mq_buf, sizeof(mq_buf));
    }
    
    if (game_started) {
        static double turn_timer = 0;
        if (turn_timer <= 0) {
            next_turn();
            int msg_size = create_msg(mq_buf, sizeof(mq_buf), T_SEND_REQ_STR, &THREAD_PROT_FMT_STR_STORAGE, get_random_required_substr());
            message_enqueue(write_queue, mq_buf, msg_size);
            turn_timer = MAX_TURN_TIMER;
            msg_size = create_msg(mq_buf, sizeof(mq_buf), T_TURN_TIME, &THREAD_PROT_FMT_STR_STORAGE, turn_timer);
            message_enqueue(write_queue, mq_buf, msg_size);
        } else {
            turn_timer -= deltatime;
        }

        // Handles how often we send to players their time left
        static double send_time_timer = 0;
        if (send_time_timer <= 0) {
            int msg_size = create_msg(mq_buf, sizeof(mq_buf), T_TURN_TIME, &THREAD_PROT_FMT_STR_STORAGE, turn_timer);
            message_enqueue(write_queue, mq_buf, msg_size);
            send_time_timer = MAX_SEND_TIMER;
        } else {
            send_time_timer -= deltatime;
        }
    }
}

// Called when "start" command issued (not the same as game_init())
static void game_start() {
    game_started = true;
    player_turn = players;
}

static void next_turn() {
    if (player_turn == NULL) {
        if (players == NULL) {
            fprintf(stderr, "next_turn() failed because there are no players.\n");
            return;
        }
        player_turn = players;
    } else {
        // Loop around to the beginning if we reach the end of players
        if (player_turn->next == NULL) {
            player_turn = players;
        } else {
            player_turn = player_turn->next;
        }
    }

    static char buf[128];
    int msg_size = create_msg(buf, sizeof(buf), T_PLAYER_TURN, &THREAD_PROT_FMT_STR_STORAGE, player_turn->id);
    message_enqueue(write_queue, buf, msg_size);
}

static struct player_data *get_player_by_id(int id) {
    struct player_data *cur = players;
    while (cur != NULL && cur->id != id) {
        cur = cur->next;
    }
    
    if (cur == NULL) {  // Given id does not belong to any player
        fprintf(stderr, "Invalid id (%d) in get_player_by_id()\n", id);
        return NULL;
    }

    return cur;
}

static void create_player_data(int id) {
    struct player_data *temp = malloc(sizeof(struct player_data));
    temp->id = id;
    //strcpy(temp->name, name);
    temp->hp = MAX_HP;
    temp->next = players;
    players = temp;
}

static void delete_player_data(int id) {
    struct player_data *cur = players;
    struct player_data *prev = NULL;
    while(cur != NULL && cur->id != id) {
        prev = cur;
        cur = cur->next;
    }

    // Given id not found
    if (cur == NULL) {
        fprintf(stderr, "id: %d not found in delete_player_data()\n", id);
        return;
    }

    if (cur == players) {
        players = cur->next;
    } else {
        prev->next = cur->next;
    }
    free(cur);
}

static void process_thread_command(char *msg) {
    char code = get_msg_code(msg);
    int len = get_msg_len(msg);
    static char temp_buf[256];

    switch(code){
        case T_PLAYER_JOIN: {
            int id;
            parse_msg_body(msg, &THREAD_PROT_FMT_STR_STORAGE, &id);      
            create_player_data(id);
            printf("Created player data (id: %d) (name: %s) in logic thread!\n", id, temp_buf);
            break;
        }
        case T_PLAYER_LEFT: {
            int id;
            parse_msg_body(msg, &THREAD_PROT_FMT_STR_STORAGE, &id);
            delete_player_data(id);
            printf("Delete player data (id: %d) in logic thread\n", id);
            break;
        }
        /*
        case T_SEND_NAME: {
            int id;
            parse_msg_body(msg, &THREAD_PROT_FMT_STR_STORAGE, &id, temp_buf);
            strcpy(get_player_by_id(id)->name, temp_buf);
            printf("Player id (%d) changed their name to %s\n", id, temp_buf);
            break;
        } */
        default:
            fprintf(stderr, "Invalid msg code in server_logic's process_thread_command() (%d)\n", code);
            break;
    }
}

// Processes commands from STDIN
static void process_terminal_command(char *cmd) {
    if (strcmp(cmd, "start") == 0) {  // Start game
        static char msg_buf[256];
        game_start();
        int msg_size = create_msg(msg_buf, sizeof(msg_buf), T_GAME_START, &THREAD_PROT_FMT_STR_STORAGE);
        message_enqueue(write_queue, msg_buf, msg_size);
    } else {  // Invalid
        fprintf(stderr, "Invalid command!\n");
    }
}
