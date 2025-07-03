#include "../include/custom_protocol.h"
#include "../include/network_header.h"

enum Msg_Values get_msg_code(char *msg){
    return (enum Msg_Values)msg[0];
}

char *get_msg_value(char *msg) {
    return msg + 1;
}
