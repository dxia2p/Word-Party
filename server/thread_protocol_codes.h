#ifndef H_THREAD_PROTOCOL_CODES
#define H_THREAD_PROTOCOL_CODES

#include "../public/custom_protocol.h"

enum Thread_Msg_Codes{
    T_INCORRECT_WORD = 0,
    T_CORRECT_WORD,
    T_SEND_WORD,
    T_SEND_REQ_STR,  // letter combination the player has to make a word with
    T_LOSE_HP,
    T_PLAYER_JOIN,
    T_PLAYER_LEFT,
    T_LEAVE,
    T_GAME_START,  // Tells networking thread that game can start
    T_TURN_TIME,  // Contains how much time the current player has to make their turn
    T_PLAYER_TURN,  // Sends the id of the next player who will take their turn
};

static const char *thread_protocol_fmtstrs[] = {      
    [T_INCORRECT_WORD] = "",
    [T_CORRECT_WORD] = "",
    [T_SEND_WORD] = "s",
    [T_SEND_REQ_STR] = "s",
    [T_LOSE_HP] = "",
    [T_PLAYER_JOIN] = "d",
    [T_PLAYER_LEFT] = "d",
    [T_LEAVE] = "",
    [T_GAME_START] = "",
    [T_TURN_TIME] = "f",
    [T_PLAYER_TURN] = "d",
};

static const struct cust_protocol_fmt_str_storage THREAD_PROT_FMT_STR_STORAGE = {
    .format_strs = thread_protocol_fmtstrs,
    .len = sizeof(thread_protocol_fmtstrs) / sizeof(thread_protocol_fmtstrs[0]),
};

#endif
