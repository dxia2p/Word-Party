#include <iso646.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "custom_protocol.h"
#include "network_header.h"


const char *get_format_string_from_code(const struct cust_protocol_fmt_str_storage *s, const char c) {
    if (c >= s->len || c < 0) {
        fprintf(stderr, "Invalid msg_code passed to get_format_string_from_code() (%d)\n", c);
    }
    return s->format_strs[c];
}


bool is_message_complete(char *msg, int msg_len) {
    for (int i = 0; i < msg_len; i++) {
        if (msg[i] == MSG_END) return true;
    }
    return false;
}


// includes msg_end in the length
int get_msg_len(char *msg) {  // TODO: FIX UNDEFINED BEHAVIOUR WHEN MSG_END IS NOT IN MSG
    return (strchr(msg, MSG_END) - msg) + 1;
}

char get_msg_code(char *msg){
    return msg[0];
}


// Pass in pointers to int, string, or float
// Make sure strings passed in are modifiable arrays
void parse_msg_body(char *msg, const struct cust_protocol_fmt_str_storage *fmt_strs, ...) {
    char code = get_msg_code(msg);
    const char *fmt = get_format_string_from_code(fmt_strs, code);

    msg++; // Skip past the code

    va_list list;
    va_start(list, fmt_strs);
    for (int i = 0; fmt[i] != '\0'; i++) {
        char temp_buf[64];
        int temp_index = 0;
        while (*msg != MSG_END && *msg != MSG_VAL_SEP) {   // TODO: Check for incorrectly formatted messages
            temp_buf[temp_index] = *msg;
            temp_index++;
            msg++;
            if (temp_index > sizeof(temp_buf) - 2) {
                temp_buf[temp_index] = '\0';
                fprintf(stderr, "value (%s) is too large for parse_msg_body()!\n", temp_buf);
                return;
            }
        }
        if (*msg == MSG_END && fmt[i + 1] != '\0') {
            fprintf(stderr, "Received message (%s) is too short for code (%d)\n", msg, code);
        }
        msg++; // Skip past MSG_VAL_SEP
        temp_buf[temp_index] = '\0';
        temp_index++;

        

        // Determine what type the argument is 
        switch (fmt[i]) {
            case 'd': {
                char *endptr;
                long res;
                res = strtol(temp_buf, &endptr, 10);
                
                if (endptr == temp_buf) {
                    // No digits were found
                    fprintf(stderr, "Could not convert (%s) to integer in parse_msg_body()\n", temp_buf);
                } else if (*endptr != '\0') {
                    // Additional characters after number
                    fprintf(stderr, "Unexpected additional characters in (%s) in parse_msg_body()\n", temp_buf);
                }

                *va_arg(list, int*) = res;
                break;
            } case 'f': {
                char *endptr;
                double res;
                res = strtod(temp_buf, &endptr);

                if (endptr == temp_buf) {
                    // No digits were found
                    fprintf(stderr, "Could not convert (%s) to double in parse_msg_body()\n", temp_buf);
                } else if (*endptr != '\0') {
                    // Additional characters after number
                    fprintf(stderr, "Unexpected additional characters in (%s) in parse_msg_body()\n", temp_buf);
                }

                *va_arg(list, double*) = res;
                break;
            } case 's': {
                strcpy(va_arg(list, char*), temp_buf);
                break;
            }
            default:
                fprintf(stderr, "Unknown format in separate_msg_body()! (%c)\n", fmt[i]);
                return;
                break;
        }
    }
}


// Returns size of msg created
// fmt should only contain s, d, or f!
int create_msg(char *buf, int buf_size, char msg_code, const struct cust_protocol_fmt_str_storage *fmt_strs, ...) {
    const char *fmt = get_format_string_from_code(fmt_strs, msg_code);
    buf[0] = msg_code;
    int buf_pos = 1;

    va_list list;
    va_start(list, fmt_strs);

    for (int i = 0; fmt[i] != '\0'; i++) {
        char arg[64];

        // Determine what type the argument is
        switch(fmt[i]) {
            case 'd': {
                int temp = va_arg(list, int);
                snprintf(arg, sizeof(arg), "%d", temp);
                break;
            }
            case 'f': {
                double temp = va_arg(list, double);
                snprintf(arg, sizeof(arg), "%f", temp);
                break;
            }
            case 's': {
                char *temp = va_arg(list, char *);
                snprintf(arg, sizeof(arg), "%s", temp);
                break;
            }
            default:
                fprintf(stderr, "Unknown format in create_msg()! (%c)\n", fmt[i]);
                return -1;
                break;
        }


        if (buf_pos + strlen(arg) + 2 > buf_size) {
            fprintf(stderr, "buffer size insufficient in create_msg_body() (size: %d)\n", buf_size);
            return -1;
        }

        if (buf_pos != 1) {
            buf[buf_pos] = MSG_VAL_SEP;
            buf_pos++;
        }
        strcpy(buf + buf_pos, arg);
        buf_pos += strlen(arg);
    }

    buf[buf_pos] = MSG_END;
    buf_pos++;
    va_end(list);
    return buf_pos;
}


void print_msg(char *msg) {
    printf("Printing msg: \n");
    while (*msg != MSG_END) {
        putchar(*msg);
        msg++;
    }
    putchar(*msg);
    putchar('\n');
}
