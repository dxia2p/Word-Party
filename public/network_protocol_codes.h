#ifndef H_NETWORK_PROTOCOL_CODES
#define H_NETWORK_PROTOCOL_CODES

#include "custom_protocol.h"

enum Network_Msg_Codes{
    N_INCORRECT_WORD = 0,  // Server sends to client
    N_CORRECT_WORD,  // Server sends to client
    N_SEND_WORD,  // <word>
    N_SEND_REQ_STR,  // Current player must use this string of letters to make a word
    N_LOSE_HP,  // server sends to client
    N_PLAYER_JOIN,  // id, <name>
    N_PLAYER_LEFT,  // id
    N_SEND_NAME,  // for client to send to server
    N_PLAYER_CHANGED_NAME,  // <id>, <name> for server to send to all clients
    N_LEAVE,  // for client to send to server
    N_GAME_FULL,  // Server sends to client
    N_GAME_NOT_FULL,  // Server sends to client
    N_GAME_START,  // Server sends to client
    N_TURN_TIME,  // Server sends to client how much time they have
    N_PLAYER_TURN,
    N_SEND_ID,  // For server to send to client, tells the client what id they are
};

static const char *net_protocol_fmtstrs[] = {      
    [N_INCORRECT_WORD] = "",
    [N_CORRECT_WORD] = "",
    [N_SEND_WORD] = "s",
    [N_SEND_REQ_STR] = "s",
    [N_LOSE_HP] = "",
    [N_PLAYER_JOIN] = "ds",
    [N_PLAYER_LEFT] = "d",
    [N_SEND_NAME] = "s",
    [N_PLAYER_CHANGED_NAME] = "ds",
    [N_LEAVE] = "",
    [N_GAME_FULL] = "",
    [N_GAME_NOT_FULL] = "",
    [N_GAME_START] = "",
    [N_TURN_TIME] = "f",
    [N_PLAYER_TURN] = "d",
    [N_SEND_ID] = "d",
};

static const struct cust_protocol_fmt_str_storage NET_PROT_FMT_STR_STORAGE = {
    .format_strs = net_protocol_fmtstrs,
    .len = sizeof(net_protocol_fmtstrs) / sizeof(net_protocol_fmtstrs[0]),
};

#endif
