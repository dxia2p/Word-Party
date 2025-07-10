#ifndef NETWORK_HANDLER_H
#define NETWORK_HANDLER_H
#include "network_header.h"

enum Recv_Status {
    R_INCOMPLETE = 0,
    R_SINGLE_COMPLETE,  // One message is ready for reading
    R_MULTIPLE_COMPLETE,  // More than one message is ready for reading
    R_DISCONNECTED  // peer has disconnected
};


int send_to_sock(SOCKET sock, char *msg, int msg_len);


struct receive_buffer;

struct receive_buffer *create_receive_buffer();
void delete_receive_buffer(struct receive_buffer *rb);
enum Recv_Status recv_from_sock(SOCKET sock, struct receive_buffer *rb, char *msg, int msg_max_len);

#endif
