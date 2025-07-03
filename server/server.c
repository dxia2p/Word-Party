#include "../include/network_header.h"
#include "../include/custom_protocol.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PLAYER_COUNT 8
#define MAX_CLIENT_BUF_SIZE 256

#define GAME_START_CMD "start"

// Function prototypes
SOCKET create_server(char *hostname, char *port);
SOCKET wait_for_client(SOCKET socket_listen);
void recv_from_client(SOCKET player_socket, char *msg, int msg_max_len);
void send_to_client(SOCKET player_socket, char *msg);
struct client_data *create_client_data(SOCKET client_socket, char *name);
void remove_client_data(SOCKET client_socket);
void send_to_all(char *msg);

struct client_data {
    int id;
    SOCKET sock;
    char name[25];
    int hp;
    struct client_data *next;
    char buf[MAX_CLIENT_BUF_SIZE];
    int bytes_received;
};

static struct client_data *clients = NULL;  // Linked list containing all clients

int main() {
    #if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
    #endif

    /* Socket setup */
    SOCKET socket_listen = create_server(0, "8080");

    /* Variables */
    int player_count = 0;

    /* Main loop */
    
    // We will be using select to handle multiplexing
    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;
    FD_SET(STDIN_FILENO, &master);
    if (STDIN_FILENO > max_socket) {
        max_socket = STDIN_FILENO;
    }

    printf("Main loop is starting...\n");
    printf("Type \"" GAME_START_CMD "\" to start the game!\n");

    while (1) {
        fd_set reads = master;  // Make copies of master
        fd_set writes = master;

        if (select(max_socket + 1, &reads, &writes, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }


        // Loop through each socket to see if it was flagged by select()
        // Not the best for performance but it shouldn't be a bottleneck
        SOCKET i;
        for (i = 1; i <= max_socket; ++i) {

            // Check for sockets open to reading
            if (FD_ISSET(i, &reads)) {

                if (i == socket_listen) {
                    SOCKET socket_client = wait_for_client(socket_listen);
                    // Turn off nagle algorithm
                    int yes = 1;
                    if (setsockopt(socket_listen, IPPROTO_TCP, TCP_NODELAY, (void*)&yes, sizeof(yes)) < 0) {
                        fprintf(stderr, "setsockopt() failed. (%d)\n", GETSOCKETERRNO());
                    }

                   
                    // HANDLE THINGS IF PLAYER COUNT IS FULL
                    if (player_count == MAX_PLAYER_COUNT) {
                        printf("Reached max player count. Dropping player (%d)...\n", socket_client);
                        char buf[2];
                        buf[0] = GAME_FULL;
                        buf[1] = '\0';
                        send_to_client(socket_client, buf);
                        CLOSESOCKET(socket_client);
                        
                    }
 
                    FD_SET(socket_client, &master);
                    if (socket_client > max_socket)
                        max_socket = socket_client;

                    player_count++;

                    // Create client_data for this player
                    create_client_data(socket_client, "Unnamed");
                    
                } else if (i != STDIN_FILENO){

                    char buf[MAX_CLIENT_BUF_SIZE];
                    recv_from_client(i, buf, sizeof(buf));
                    
                    // TODO: PROCESS CLIENT COMMANDS
                        
                } else {  // STDIN_FILNO
                    char server_input[64];
                    scanf("%63s", server_input);
                    getchar();

                    if (strcmp(server_input, GAME_START_CMD) == 0) {
                        printf("Starting game...\n");
                        // do stuff to start the game
                    } else {  // Invalid command
                        printf("Invalid command!\n");
                    }
                }
            }
            
            // Check for sockets open to writing
            if (FD_ISSET(i, &writes)) {

            }
        }
    }


    /* Cleanup */
    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

#if defined(_WIN32)
    WSACleanup();
#endif
    
    return 0;
}



SOCKET create_server(char *hostname, char *port) {
    printf("Configuring server address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    int getaddrinfo_code = getaddrinfo(0, "8080", &hints, &bind_address);
    if (getaddrinfo_code != 0) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", getaddrinfo_code);
        return 1;
    }

    printf("Creating socket...\n");
    SOCKET socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Binding socket...\n");
    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);
    
    printf("Listening for connections...\n");
    if (listen(socket_listen, 8) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    return socket_listen;
}


SOCKET wait_for_client(SOCKET socket_listen) {
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    SOCKET socket_client = accept(socket_listen, (struct sockaddr*) &client_address, &client_len);

    if(!ISVALIDSOCKET(socket_client)) {
        fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
                    
    char address_buffer[100];
    getnameinfo((struct sockaddr*)&client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    printf("%s has connected.\n", address_buffer);
    return socket_client;
}


void recv_from_client(SOCKET player_socket, char *msg, int msg_max_len) {  // TODO: Fix this function so that it puts received data in a buffer stored in client_data structure
    int bytes_recv = 0;
    msg[0] = '\0';

    do {
        int msg_bytes = recv(player_socket, msg + bytes_recv, msg_max_len - bytes_recv, 0);  // Don't need to -1 from msg_max_len because the last char ('$') is replaced with null char
        
        if (msg_bytes == 0) {
            fprintf(stderr, "Unexpected disconnect from socket (%d).\n", player_socket);
            // MAKE SURE TO HANDLE PLAYER DISCONNECT
            //remove_client_data(player_socket);
            return;
        }
        if (msg_bytes == -1) {
            fprintf(stderr, "recv_from_player() failed. (%d)\n", GETSOCKETERRNO());
            exit(1);
        }
        bytes_recv += msg_bytes;
    } while (msg[bytes_recv - 1] != MSG_END);
        
    *strchr(msg, MSG_END) = '\0';
}

void send_to_client(SOCKET player_socket, char *msg) {
    int msg_len = strlen(msg) + 1;
    msg[msg_len - 1] = '$';  // overwrite null terminator
    int bytes_sent = 0;
    do {
        int msg_bytes = send(player_socket, msg + bytes_sent, msg_len - bytes_sent, 0);
        

        if (msg_bytes == -1) {
            fprintf(stderr, "send_to_player() failed. (%d)\n", GETSOCKETERRNO());
            exit(1);
        }
        bytes_sent += msg_bytes;

    } while (bytes_sent < msg_len);
}

struct client_data *create_client_data(SOCKET client_socket, char *name) {
    static int cur_id = 0;
    struct client_data *temp = calloc(1, sizeof(struct client_data));
    temp->hp = 2;
    temp->id = cur_id++;
    strcpy(temp->name, name);
    temp->sock = client_socket;
    temp->bytes_received = 0;

    // Add new client to beginning of linked list
    temp->next = clients;
    clients = temp;

    printf("Created client (name: %s, socket: %d).\n", name, client_socket);

    return temp;
}

// Remember to close the socket and remove from fd_set after using this function!
void remove_client_data(SOCKET client_socket) {

    struct client_data *prev = NULL;
    struct client_data *cur = clients;

    while (cur->sock != client_socket && cur != NULL) {
        prev = cur;
        cur = cur->next;
    }

    if (cur == NULL) {  // the specified socket does not exist! (this should never happen)
        fprintf(stderr, "destroy_client() could not find socket (%d).\n", client_socket);
        return;
    }

    if (prev == NULL) {  // we are removing the first element of the clients linked list
        clients = clients->next;
        free(cur);
    } else {
        prev->next = cur->next;
        free(cur);
    }
    
    printf("Removed socket (%d).\n", client_socket);
}

void send_to_all(char *msg) {
    struct client_data *cur = clients;
    while (cur != NULL) {
        send_to_client(cur->sock, msg);
        cur = cur->next;
    }
}
