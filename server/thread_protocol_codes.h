#ifndef H_THREAD_PROTOCOL_CODES
#define H_THREAD_PROTOCOL_CODES

enum Thread_Msg_Codes{
    T_INCORRECT_WORD = 0,
    T_CORRECT_WORD,
    T_SEND_WORD,
    T_LOSE_HP,
    T_PLAYER_JOIN,
    T_PLAYER_LEFT,
    T_SEND_NAME,  // Player changes their name
    T_LEAVE,
    T_GAME_START,
};

static const char *thread_protocol_fmtstrs[] = {      
    [T_INCORRECT_WORD] = "",
    [T_CORRECT_WORD] = "",
    [T_SEND_WORD] = "s",
    [T_LOSE_HP] = "",
    [T_PLAYER_JOIN] = "ds",
    [T_PLAYER_LEFT] = "d",
    [T_SEND_NAME] = "ds",
    [T_LEAVE] = "",
    [T_GAME_START] = "",
};

#endif
