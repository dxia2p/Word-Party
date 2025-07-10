#ifndef CUSTOM_PROTOCOL_H
#define CUSTOM_PROTOCOL_H

#include <stdbool.h>


enum Msg_Codes{
    MSG_END = '$',
    MSG_VAL_SEP = '&',
    WORD_INCORRECT = 0,  // Server sends to client
    WORD_CORRECT,  // Server sends to client
    SEND_WORD,  // <word>
    LOSE_HP,  // server sends to client
    PLAYER_JOIN,  // id, <name>
    PLAYER_LEFT,  // id
    SEND_NAME,  // <name> for client to send to server
    PLAYER_CHANGED_NAME,  // <id>, <name> for server to send to all clients
    LEAVE,  // for client to send to server
    GAME_FULL,  // Server sends to client
    GAME_NOT_FULL,  // Server sends to client
    GAME_START,  // Server sends to client
};

const char *get_format_string_from_code(enum Msg_Codes c);


enum Msg_Codes get_msg_code(char *msg);
bool is_message_complete(char *msg, int msg_len);
int get_msg_len(char *msg);
//int separate_msg_body(char *msg, char values_buf[][50], int values_buf_size);  // BODY SHOULD BE AN ENTIRE UNMODIFIED MESSAGE!
void parse_msg_body(char *msg, ...);
int create_msg(char *buf, int buf_size, enum Msg_Codes msg_code, ...);
void print_msg(char *msg);  // for debugging

#endif
