#include "../public/network_header.h"
#include "../public/custom_protocol.h"
#include "../public/network_handler.h"
#include "word_validator.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PLAYER_COUNT 8
//#define MAX_CLIENT_BUF_SIZE 256

#define GAME_START_CMD "start"
#define QUIT_CMD "quit"

// Function prototypes
SOCKET create_server(char *hostname, char *port);
SOCKET wait_for_client(SOCKET socket_listen);
//int recv_from_client(SOCKET client_socket, char *msg);
//void send_to_client(SOCKET player_socket, char *msg, int msg_len);
struct client_data *create_client_data(SOCKET client_socket, char *name);
void remove_client_data(SOCKET client_socket);
struct client_data *get_client_data_from_socket(SOCKET client_socket);
void send_to_all(char *msg, int msg_len);
void send_to_all_except_one(char *msg, int msg_len, SOCKET except_this_socket);
void process_client_command(char *msg, struct client_data *cd);

struct client_data {
    SOCKET sock;  // client's socket descriptor doubles as an id
    char name[25];
    int hp;
    struct client_data *next;
    struct receive_buffer *rb;
    //char recv_buf[MAX_CLIENT_BUF_SIZE];
    //int bytes_received;
    //char send_buf[MAX_CLIENT_BUF_SIZE];
    //int bytes_to_send;
};

static struct client_data *clients = NULL;  // Linked list containing all clients
static int player_count = 0;

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
    printf("Type \"" GAME_START_CMD "\" to start the game, or " QUIT_CMD " to quit.\n");

    char msg_buf[512];
    char recv_buf[512];

    while (1) {
        fd_set reads = master;  // Make copies of master
        //fd_set writes = master;

        if (select(max_socket + 1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        if (FD_ISSET(STDIN_FILENO, &reads)) {   // Input from console
            char server_input[64];
            scanf("%63s", server_input);
            getchar();
            
            if (strcmp(server_input, GAME_START_CMD) == 0) {
                printf("Starting game...\n");
                // do stuff to start the game
            } else if (strcmp(server_input, QUIT_CMD) == 0) {
                printf("Quitting...\n");
                break;
            }
            else {  // Invalid command
                printf("Invalid command!\n");
            }
        }

        if (FD_ISSET(socket_listen, &reads)) {  // New client ready to join
            SOCKET socket_client = wait_for_client(socket_listen);
            // Turn off nagle algorithm
            int yes = 1;
            if (setsockopt(socket_listen, IPPROTO_TCP, TCP_NODELAY, (void*)&yes, sizeof(yes)) < 0) {
                fprintf(stderr, "setsockopt() failed. (%d)\n", GETSOCKETERRNO());
            }

            // HANDLE THINGS IF PLAYER COUNT IS FULL
            if (player_count == MAX_PLAYER_COUNT) {
                printf("Reached max player count. Dropping player (%d)...\n", socket_client);

                int msg_size = create_msg(msg_buf, sizeof(msg_buf), GAME_FULL);
                send_to_sock(socket_client, msg_buf, msg_size);
                CLOSESOCKET(socket_client);
                continue;
            }
            
            int msg_size = create_msg(msg_buf, sizeof(msg_buf), GAME_NOT_FULL);
            //print_msg(msg_buf);
            send_to_sock(socket_client, msg_buf, msg_size);

            FD_SET(socket_client, &master);
            if (socket_client > max_socket)
                max_socket = socket_client;

            player_count++;

            // Create client_data for this player
            struct client_data *new_client = create_client_data(socket_client, "Unnamed");
            msg_size = create_msg(msg_buf, sizeof(msg_buf), PLAYER_JOIN, new_client->sock, new_client->name);
            printf("Creating client data...\n");
            //print_msg(msg_buf);
            send_to_all(msg_buf, msg_size);

            // Send the names of all existing players to the new player 
            struct client_data *cur = clients;
            while (cur != NULL) {
                if (cur->sock == socket_client) {  // Don't send the new client's name to themselves
                    cur = cur->next;
                    continue;
                }  
                msg_size = create_msg(msg_buf, sizeof(msg_buf), PLAYER_JOIN, cur->sock, cur->name);
                //print_msg(msg_buf);
                send_to_sock(socket_client, msg_buf, msg_size);
                cur = cur->next;
            }
        }

        // Loop through all clients to see if they are ready for reading
        struct client_data *cur = clients;
        while (cur != NULL) {
            if (FD_ISSET(cur->sock, &reads)) {  // Client's socket is ready for reading
                
                enum Recv_Status status = recv_from_sock(cur->sock, cur->rb, recv_buf, sizeof(recv_buf));
                while (status != R_INCOMPLETE && status != R_DISCONNECTED) {  // Keep reading until there is nothing left to read
                    process_client_command(recv_buf, cur);
                    printf("%d: %s\n", cur->sock, recv_buf);
                    if (status == R_SINGLE_COMPLETE) {
                        break;
                    }
                    status = recv_from_sock(cur->sock, cur->rb, recv_buf, sizeof(recv_buf));
                }
                if (status == R_DISCONNECTED) {
                    int msg_size = create_msg(msg_buf, sizeof(msg_buf), PLAYER_LEFT, cur->sock);
                    send_to_all_except_one(msg_buf, msg_size, cur->sock);
                    FD_CLR(cur->sock, &master);
                    remove_client_data(cur->sock);
                    CLOSESOCKET(cur->sock);
                    player_count--;
                }
            }
            cur = cur->next;
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
        exit(1);
    }

    printf("Creating socket...\n");
    SOCKET socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    // for debugging only! -----------------------------------------
    int opt = 1;
    setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // -------------------------------------------------------------


    printf("Binding socket...\n");
    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }
    freeaddrinfo(bind_address);
    
    printf("Listening for connections...\n");
    if (listen(socket_listen, 8) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
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


/*
// parameter msg should have a size larger than or equal to MAX_CLIENT_BUF_SIZE
// return values: -1 = client has disconencted
// 0 = Has not received complete message
// 1 = received complete message

int recv_from_client(SOCKET client_socket, char *msg) {
    // Find the correct client_data corresponding to client_socket
    struct client_data *cd = get_client_data_from_socket(client_socket);
    
    // Check if there is already a complete message in cd->recv_buf
    if (is_message_complete(cd->recv_buf, cd->bytes_received)) {
        int len = get_msg_len(cd->recv_buf);  // len is the index of MSG_END
        strncpy(msg, cd->recv_buf, len);
        if (len != MAX_CLIENT_BUF_SIZE) {
            memmove(cd->recv_buf, cd->recv_buf + len, MAX_CLIENT_BUF_SIZE - len);
        }
        cd->bytes_received -= len;
        return 1;
    }

    // receive data from client
    int msg_bytes = recv(client_socket, cd->recv_buf + cd->bytes_received, MAX_CLIENT_BUF_SIZE - cd->bytes_received, 0);
    if (msg_bytes == 0) {
        return -1;
    }
    cd->bytes_received += msg_bytes;

    // Check if the message is complete
    // If it is, return true and put the message in char *msg
    // If not, return false and do nothing to char *msg
    if (is_message_complete(cd->recv_buf, cd->bytes_received)) {
        int len = get_msg_len(cd->recv_buf);  // len is the index of MSG_END
        strncpy(msg, cd->recv_buf, len);
        if (len != MAX_CLIENT_BUF_SIZE) {
            memmove(cd->recv_buf, cd->recv_buf + len, MAX_CLIENT_BUF_SIZE - len);
        }
        cd->bytes_received -= len;
        return 1;
    }
    return 0;
}
*/


/*
// Use custom_protocol.h's create_msg() to format the message properly
void send_to_client(SOCKET player_socket, char *msg, int msg_len) {
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
*/

struct client_data *create_client_data(SOCKET client_socket, char *name) {
    struct client_data *temp = calloc(1, sizeof(struct client_data));
    temp->hp = 2;
    strcpy(temp->name, name);
    temp->sock = client_socket;
    temp->rb = create_receive_buffer();

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

    while (cur != NULL && cur->sock != client_socket) {
        prev = cur;
        cur = cur->next;
    }

    if (cur == NULL) {  // the specified socket does not exist! (this should never happen)
        fprintf(stderr, "destroy_client() could not find socket (%d).\n", client_socket);
        return;
    }

    if (prev == NULL) {  // we are removing the first element of the clients linked list
        clients = clients->next;
        delete_receive_buffer(cur->rb);
        free(cur);
    } else {
        prev->next = cur->next;
        delete_receive_buffer(cur->rb);
        free(cur);
    }
    
    printf("Removed socket (%d).\n", client_socket);
}

struct client_data *get_client_data_from_socket(SOCKET client_socket) {
    struct client_data *cur = clients;

    while (cur != NULL) {
        if (cur->sock == client_socket) {
            return cur;
        }
        cur = cur->next;
    }
    fprintf(stderr, "get_client_data_from_socket() could not find socket (%d).\n", client_socket);
    return NULL;
}

void send_to_all(char *msg, int msg_len) {
    struct client_data *cur = clients;
    while (cur != NULL) {
        send_to_sock(cur->sock, msg, msg_len);
        cur = cur->next;
    }
}

void send_to_all_except_one(char *msg, int msg_len, SOCKET except_this_socket) {
    struct client_data *cur = clients;
    while(cur != NULL) {
        if (cur->sock == except_this_socket){
            cur = cur->next;
            continue;
        }
        send_to_sock(cur->sock, msg, msg_len);
        cur = cur->next;
    }
}

// cd is the client_data for the client that send this message
void process_client_command(char *msg, struct client_data *cd) {
    char code = get_msg_code(msg);
    static char temp_buf[512];

    switch(code){
        case SEND_WORD:
            break;
        case SEND_NAME: {
            char new_name[32];
            parse_msg_body(msg, new_name);
            strcpy(cd->name, new_name);
            int msg_size = create_msg(temp_buf, sizeof(temp_buf), PLAYER_CHANGED_NAME, cd->sock, cd->name);
            send_to_all(temp_buf, msg_size);
            printf("socket (%d) has new name (%s).\n", cd->sock, cd->name);
            break;
        }
        default:
            fprintf(stderr, "Invalid message code! (%d)\n", code);
            break;
    }
}
