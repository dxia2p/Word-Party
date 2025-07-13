#ifndef CUSTOM_PROTOCOL_H
#define CUSTOM_PROTOCOL_H

#include <stdbool.h>

/* 
    * The first 8 bits of a message contains a code which will identify what type of data it contains
    * Messages end with MSG_END and elements in the body are separated by MSG_VAL_SEP
    * How do I use this custom protocol?
    * First, make an enum with the unique messaging codes you want your protocol to have
    * Next, make an array of strings
    * Set each element in the array corresponding to a messaging code to a format string (containing s, d, or f)
    * This format string array will ensure parsing functions know what type each element in the body is supposed to be
*/

enum Message_Boundaries {
    MSG_END = '$',
    MSG_VAL_SEP = '&',
};

const char *get_format_string_from_code(char **protocol_fmt_strs, int fmt_strs_size, char c);
char get_msg_code(char *msg);
bool is_message_complete(char *msg, int msg_len);
int get_msg_len(char *msg);
void parse_msg_body(char *msg, char**protocol_fmt_strs, int fmt_strs_size, ...);
int create_msg(char *buf, int buf_size, char msg_code, char **protocol_fmt_strs, int fmt_strs_len, ...);
void print_msg(char *msg);

#endif
