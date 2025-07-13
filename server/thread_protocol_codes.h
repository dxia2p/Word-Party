#ifndef H_THREAD_PROTOCOL_CODES
#define H_THREAD_PROTOCOL_CODES

enum Network_Msg_Codes{
    T_INCORRECT_WORD = 0,
    T_CORRECT_WORD,
    T_SEND_WORD,
    T_LOSE_HP,
    T_PLAYER_JOIN,
    T_PLAYER_LEFT,
    T_SEND_NAME,
    T_PLAYER_CHANGED_NAME,
    T_LEAVE,
    T_GAME_START,
};

static const char *protocol_format_strs[] = {      
    [T_INCORRECT_WORD] = "",
    [T_CORRECT_WORD] = "",
    [T_SEND_WORD] = "s",
    [T_LOSE_HP] = "",
    [T_PLAYER_JOIN] = "ds",
    [T_PLAYER_LEFT] = "d",
    [T_SEND_NAME] = "s",
    [T_PLAYER_CHANGED_NAME] = "ds",
    [T_LEAVE] = "",
    [T_GAME_START] = "",
};

#endif
