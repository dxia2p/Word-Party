#ifndef CUSTOM_PROTOCOL_H
#define CUSTOM_PROTOCOL_H

// Protocol:
// Everything is terminated by $
// 0: Word is incorrect
// 1: Word is correct
// 2<word>: send word to server
// 3: player loses one hp
// 4<name>: player joins, submits name
// 5<name>: player leaves
// 6: game full
// 7: game not full


enum Msg_Values{
    MSG_END = '$',
    WORD_INCORRECT = 0,
    WORD_CORRECT,
    SEND_WORD,  // <word>
    LOSE_HP,
    PLAYER_JOIN,  // <name>
    PLAYER_LEFT,  // <name>
    GAME_FULL,
    GAME_NOT_FULL,
};


enum Msg_Values get_msg_code(char *msg);
char *get_msg_value(char *msg);

#endif
