#include "../include/network_header.h"
#include "../include/custom_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Function prototypes */
SOCKET connect_to_server(char *hostname, char *port);
void recv_from_server(SOCKET server_socket, char *msg, int msg_max_len);
void send_to_server(SOCKET server_socket, char *msg);




int main() {
    #if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
    #endif

    // Obtain server's address
    char hostname[100];
    char port[6];
    printf("Enter hostname: ");
    scanf("%99s", hostname);
    getchar();
    printf("Enter port: ");
    scanf("%5s", port);
    getchar();

    SOCKET socket_serv = connect_to_server(hostname, port);

    // Turn off nagles algorithm
    int yes = 1;
    if (setsockopt(socket_serv, IPPROTO_TCP, TCP_NODELAY, (void*)&yes, sizeof(yes)) < 0) {
        fprintf(stderr, "setsockopt() failed. (%d)\n", GETSOCKETERRNO());
    }
    

    char recv_buffer[500];
    recv_from_server(socket_serv, recv_buffer, sizeof(recv_buffer));
    if (get_msg_code(recv_buffer) == GAME_FULL){
        fprintf(stderr, "Game is full!\n");
        return 1;
    }
    printf("Joining game...\n");

    // At this point we should be successfully connected
    char my_name[26];
    my_name[0] = PLAYER_JOIN;
    printf("Enter your name (max 24 char): ");
    scanf("%24s", my_name + 1);
    getchar();
    send_to_server(socket_serv, my_name);
    printf("Submitted name to server!\n");

    

    /* cleanup */
    printf("Closing socket...\n");
    CLOSESOCKET(socket_serv);

#if defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}



SOCKET connect_to_server(char *hostname, char *port) {
    printf("Configuring address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *server_address;
    if (getaddrinfo(hostname, port, &hints, &server_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Creating socket...\n");
    SOCKET socket_serv = socket(server_address->ai_family, server_address->ai_socktype, server_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_serv)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Connecting to server...\n");
    if (connect(socket_serv, server_address->ai_addr, server_address->ai_addrlen) != 0) {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(server_address);

    return socket_serv;
}


void recv_from_server(SOCKET server_socket, char *msg, int msg_max_len) {
    int bytes_recv = 0;
    msg[0] = '\0';

    do {
        int msg_bytes = recv(server_socket, msg + bytes_recv, msg_max_len - bytes_recv, 0);  // Don't need to -1 from msg_max_len because the last char ('$') is replaced with null char

        
        if (msg_bytes == 0) {

            fprintf(stderr, "Unexpected shutdown.\n");
            exit(1);
        }
        if (msg_bytes == -1) {
            fprintf(stderr, "recv_from_server() failed. (%d)\n", GETSOCKETERRNO());
            exit(1);
        }
        bytes_recv += msg_bytes;
    } while (msg[bytes_recv - 1] != MSG_END);
        
    
    *strchr(msg, MSG_END) = '\0';
}

/* char *msg is a null terminated string */
void send_to_server(SOCKET server_socket, char *msg) {
    int msg_len = strlen(msg) + 1;
    msg[msg_len - 1] = '$';  // overwrite null terminator
    int bytes_sent = 0;
    do {
        int msg_bytes = send(server_socket, msg + bytes_sent, msg_len, 0);
        bytes_sent += msg_bytes;

        if (msg_bytes == -1) {
            fprintf(stderr, "send_to_server() failed. (%d)\n", GETSOCKETERRNO());
            exit(1);
        }

    } while (bytes_sent < msg_len);
    
}
