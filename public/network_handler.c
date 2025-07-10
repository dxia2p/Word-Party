#include "network_header.h"
#include "network_handler.h"
#include "custom_protocol.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_RECV_BUFFER_SIZE 512


int send_to_sock(SOCKET sock, char *msg, int msg_len) {
    int bytes_sent = 0;
    do {
        int msg_bytes = send(sock, msg + bytes_sent, msg_len - bytes_sent, 0);

        if (msg_bytes == -1) {
            fprintf(stderr, "send_to_sock() failed. (%d)\n", GETSOCKETERRNO());
            return -1;
        }
        bytes_sent += msg_bytes;

    } while (bytes_sent < msg_len);
    return 0;
}

struct receive_buffer {
    char received[MAX_RECV_BUFFER_SIZE];
    int bytes_received;
};

struct receive_buffer *create_receive_buffer() {
    return calloc(1, sizeof(struct receive_buffer));
}

void delete_receive_buffer(struct receive_buffer *rb) {
    free (rb);
}

// This function assumes that the socket is ready to be read from
enum Recv_Status recv_from_sock(SOCKET sock, struct receive_buffer *rb, char *msg, int msg_max_len) {     
    // check if there is already a complete message in the buffer (in case we received 2 or more complete messages combined into 1
    if (is_message_complete(rb->received, rb->bytes_received)) {
        int len = get_msg_len(rb->received);
        strncpy(msg, rb->received, len);
        if (len != sizeof(rb->received)) {
            memmove(rb->received, rb->received + len, sizeof(rb->received) - len);  // TODO: CLEAR STUFF AFTER MOVING IT
            memset(rb->received + (rb->bytes_received - len), 0, sizeof(rb->received) - (rb->bytes_received - len));
        }
        rb->bytes_received -= len;
        if (is_message_complete(rb->received, rb->bytes_received)) {
            return R_MULTIPLE_COMPLETE;
        }
        
        return R_SINGLE_COMPLETE;
    }
    
    int msg_bytes = recv(sock, rb->received + rb->bytes_received, sizeof(rb->received) - rb->bytes_received, 0);
    if (msg_bytes == 0) {
        fprintf(stderr, "Unexpected shutdown from socket %d.\n", sock);
        return R_DISCONNECTED;
    }
    rb->bytes_received += msg_bytes;


//printf("recv_from_sock contents (%d bytes): %.*s\n", rb->bytes_received, rb->byte_received, rb->received);
    
    // Check if the message is complete
        if (is_message_complete(rb->received, rb->bytes_received)) {
        int len = get_msg_len(rb->received);
        strncpy(msg, rb->received, len);
        if (len != sizeof(rb->received)) {
            memmove(rb->received, rb->received + len, sizeof(rb->received) - len);  // TODO: CLEAR STUFF AFTER MOVING IT
            memset(rb->received + (rb->bytes_received - len), 0, sizeof(rb->received) - (rb->bytes_received - len));
        }
        rb->bytes_received -= len;
        if (is_message_complete(rb->received, rb->bytes_received)) {
            return R_MULTIPLE_COMPLETE;
        }
        
        return R_SINGLE_COMPLETE;
    }


    return R_INCOMPLETE;
}
