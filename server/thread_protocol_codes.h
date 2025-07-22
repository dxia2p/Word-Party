#ifndef H_THREAD_PROTOCOL_CODES
#define H_THREAD_PROTOCOL_CODES

#include "../public/custom_protocol.h"

enum Thread_Msg_Codes{
    T_INCORRECT_WORD = 0,
    T_CORRECT_WORD,
    T_SEND_WORD,
    T_SEND_REQ_STR,  // letter combination the player has to make a word with
    T_LOSE_HP,  // Sends id of player who will lose 1 hp
    T_PLAYER_JOIN,
    T_PLAYER_LEFT,
    T_GAME_START,  // Tells networking thread that game can start
    T_TURN_TIME,  // Contains how much time the current player has to make their turn
    T_PLAYER_TURN,  // Sends the id of the next player who will take their turn
    T_PLAYER_WON,
    T_RESTART_GAME,
    T_GAME_STARTED_NO_JOINING,
};

static const char *thread_protocol_fmtstrs[] = {      
    [T_INCORRECT_WORD] = "d",
    [T_CORRECT_WORD] = "ds",
    [T_SEND_WORD] = "ds",
    [T_SEND_REQ_STR] = "s",
    [T_LOSE_HP] = "d",
    [T_PLAYER_JOIN] = "d",
    [T_PLAYER_LEFT] = "d",
    [T_GAME_START] = "",
    [T_TURN_TIME] = "f",
    [T_PLAYER_TURN] = "d",
    [T_PLAYER_WON] = "d",
    [T_RESTART_GAME] = "",
    [T_GAME_STARTED_NO_JOINING] = "d",
};

static const struct cust_protocol_fmt_str_storage THREAD_PROT_FMT_STR_STORAGE = {
    .format_strs = thread_protocol_fmtstrs,
    .len = sizeof(thread_protocol_fmtstrs) / sizeof(thread_protocol_fmtstrs[0]),
};

#endif
