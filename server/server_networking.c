#include "../public/network_header.h"
#include "../public/network_handler.h"
#include "../public/custom_protocol.h"
#include "../public/network_protocol_codes.h"
#include "server_networking.h"
#include "message_queue.h"
#include "thread_protocol_codes.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_CLIENT_COUNT 8

/* struct */
struct client_data {
    SOCKET sock;   // same as id in server_logic's player_data
    struct receive_buffer *rb;
    char name[26];
    struct client_data *next;
};

static struct client_data *clients = NULL;
static int client_count = 0;
static struct message_queue *write_queue;
static struct message_queue *read_queue;

// Private functions
static void process_client_command(char *msg, struct client_data *cd);
static void process_thread_command(char *msg);
static void remove_client_data(SOCKET client_socket);

/* Main loop */
void* networking_main_routine(void *queue_group) {
    #if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
    #endif

    read_queue = ((struct message_queue_group*)queue_group)->read_q;
    write_queue = ((struct message_queue_group*)queue_group)->write_q;

    SOCKET socket_listen = create_server(0, "8080");

    // We will be using select to handle multiplexing
    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;
    FD_SET(get_read_pipe(read_queue), &master);
    if (get_read_pipe(read_queue) > max_socket) {
        max_socket = get_read_pipe(read_queue);
    }

    char mq_buf[512];
    char recv_buf[512];
    char send_buf[512];

    printf("Starting networking loop...\n");

    while(1) {
        fd_set reads = master;
        if (select(max_socket + 1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return NULL;
        }

        // There are messages in the message queue
        if (FD_ISSET(get_read_pipe(read_queue), &reads)) {
            while(message_dequeue(read_queue, mq_buf, sizeof(mq_buf)) != 0) {
                process_thread_command(mq_buf);
            }
        }

        // A new client is ready to join
        if (FD_ISSET(socket_listen, &reads)) {
            SOCKET client_socket = wait_for_client(socket_listen);
            // PLAYER COUNT IS FULL
            if (client_count == MAX_CLIENT_COUNT) {
                printf("Reached max player count. Dropping player (%d)...\n", client_socket);
                int msg_size = create_msg(send_buf, sizeof(send_buf), N_GAME_FULL, &NET_PROT_FMT_STR_STORAGE);
                send_to_sock(client_socket, send_buf, msg_size);
                CLOSESOCKET(client_socket);
                continue;
            }
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_GAME_NOT_FULL, &NET_PROT_FMT_STR_STORAGE);
            send_to_sock(client_socket, send_buf, msg_size);

            FD_SET(client_socket, &master);
            if(client_socket > max_socket) {
                max_socket = client_socket;
            }

            printf("NEW CLIENT HAS JOINED!!!!\n");
            msg_size = create_msg(send_buf, sizeof(send_buf), T_PLAYER_JOIN, &THREAD_PROT_FMT_STR_STORAGE, client_socket);
            message_enqueue(write_queue, send_buf, msg_size);

            // Send the names of all existing players to the new player 
            struct client_data *cur = clients;
            while (cur != NULL) {
                msg_size = create_msg(send_buf, sizeof(send_buf), N_PLAYER_JOIN, &NET_PROT_FMT_STR_STORAGE, cur->sock, cur->name);
                //print_msg(msg_buf);
                send_to_sock(client_socket, send_buf, msg_size);
                cur = cur->next;
            }

            create_client_data(client_socket);
            client_count++;

            // Tell everyone new player has joined
            msg_size = create_msg(send_buf, sizeof(send_buf), N_PLAYER_JOIN, &NET_PROT_FMT_STR_STORAGE, client_socket, "Unnamed");
            send_to_all(send_buf, msg_size);

            // Send the new client their id
            msg_size = create_msg(send_buf, sizeof(send_buf), N_SEND_ID, &NET_PROT_FMT_STR_STORAGE, client_socket);
            send_to_sock(client_socket, send_buf, msg_size);
        }

        struct client_data *cur = clients;
        while (cur != NULL) {
            if (FD_ISSET(cur->sock, &reads)) {
                // Handle client message
                enum Recv_Status status = recv_from_sock(cur->sock, cur->rb, recv_buf, sizeof(recv_buf));
                while (status != R_INCOMPLETE && status != R_DISCONNECTED) {  // Keep reading until there is nothing left to read
                    process_client_command(recv_buf, cur);
                    if (status == R_SINGLE_COMPLETE) {
                        break;
                    }
                    status = recv_from_sock(cur->sock, cur->rb, recv_buf, sizeof(recv_buf));
                }
                if (status == R_DISCONNECTED) {
                    int msg_size = create_msg(send_buf, sizeof(send_buf), N_PLAYER_LEFT, &NET_PROT_FMT_STR_STORAGE, cur->sock);
                    send_to_all_except_one(send_buf, msg_size, cur->sock);
                    msg_size = create_msg(send_buf, sizeof(send_buf), T_PLAYER_LEFT, &THREAD_PROT_FMT_STR_STORAGE, cur->sock);
                    message_enqueue(write_queue, send_buf, msg_size);
                    FD_CLR(cur->sock, &master);
                    remove_client_data(cur->sock);
                    CLOSESOCKET(cur->sock);
                    client_count--;
                }
            }
            cur = cur->next;
        }
    }
}

/* Functions */
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
    
    char host[64];
    char service[64];
    getnameinfo(bind_address->ai_addr, bind_address->ai_addrlen, host, sizeof(host), service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV);
    printf("Server is starting on %s:%s\n", host, service);

    printf("Creating socket...\n");
    SOCKET socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    // for debugging only! ---------------------------------------------------
    int opt = 1;
    setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // -----------------------------------------------------------------------


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

struct client_data *create_client_data(SOCKET s) {
    struct client_data *temp = calloc(1, sizeof(struct client_data));
    temp->sock = s;
    temp->rb = create_receive_buffer();
    temp->next = NULL; 

    if (clients == NULL) {
        clients = temp;
        return temp;
    }

    struct client_data *cur = clients;
    while (cur->next != NULL) {
        cur = cur->next;
    }
    cur->next = temp;
    return temp;
}

// Remember to close the socket and remove from fd_set after using this function!
static void remove_client_data(SOCKET client_socket) {

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

static void process_client_command(char *msg, struct client_data *cd) {
    char code = get_msg_code(msg);
    int len = get_msg_len(msg);
    static char send_buf[256];
    static char recv_str[256];

    switch(code){
        case N_SEND_NAME: {
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, recv_str);
            strcpy(cd->name, recv_str);
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_PLAYER_CHANGED_NAME, &NET_PROT_FMT_STR_STORAGE, cd->sock, recv_str);
            send_to_all(send_buf, msg_size);
            break;
        } case N_SEND_WORD: {
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, recv_str);
            int msg_size = create_msg(send_buf, sizeof(send_buf), T_SEND_WORD, &THREAD_PROT_FMT_STR_STORAGE, cd->sock, recv_str);
            message_enqueue(write_queue, send_buf, msg_size);
            break;
        }
        default:
            fprintf(stderr, "Invalid msg code in process_client_command() (%d)\n", code);
            break;
    }
}

static void process_thread_command(char *msg) {
    char code = get_msg_code(msg);
    int len = get_msg_len(msg);
    static char recv_str[256];
    static char send_buf[256];

    switch (code) {
        case T_GAME_START: {  // Game start command from logic thread
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_GAME_START, &NET_PROT_FMT_STR_STORAGE);
            send_to_all(send_buf, msg_size);
            break;
        }
        case T_SEND_REQ_STR: {
            parse_msg_body(msg, &THREAD_PROT_FMT_STR_STORAGE, recv_str);
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_SEND_REQ_STR, &NET_PROT_FMT_STR_STORAGE, recv_str);
            send_to_all(send_buf, msg_size);
            break;
        }
        case T_TURN_TIME: {
            double t;
            parse_msg_body(msg, &THREAD_PROT_FMT_STR_STORAGE, &t);
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_TURN_TIME, &NET_PROT_FMT_STR_STORAGE, t);
            send_to_all(send_buf, msg_size);
            break;
        }
        case T_PLAYER_TURN: {
            int id;
            parse_msg_body(msg, &THREAD_PROT_FMT_STR_STORAGE, &id);
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_PLAYER_TURN, &NET_PROT_FMT_STR_STORAGE, id);
            send_to_all(send_buf, msg_size);
            break;
        } case T_CORRECT_WORD: {
            int id;
            parse_msg_body(msg, &THREAD_PROT_FMT_STR_STORAGE, &id, recv_str);
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_CORRECT_WORD, &NET_PROT_FMT_STR_STORAGE, id, recv_str);
            send_to_all(send_buf, msg_size);
            break;
        }
        case T_INCORRECT_WORD: {
            int id;
            parse_msg_body(msg, &THREAD_PROT_FMT_STR_STORAGE, &id);
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_INCORRECT_WORD, &NET_PROT_FMT_STR_STORAGE, id);
            send_to_all(send_buf, msg_size);
            break;
        } case T_LOSE_HP: {
            int id;
            parse_msg_body(msg, &THREAD_PROT_FMT_STR_STORAGE, &id);
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_LOSE_HP, &NET_PROT_FMT_STR_STORAGE, id);
            send_to_all(send_buf, msg_size);
            break;
        } case T_PLAYER_WON: {
            int id;
            parse_msg_body(msg, &THREAD_PROT_FMT_STR_STORAGE, &id);
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_PLAYER_WON, &NET_PROT_FMT_STR_STORAGE, id);
            send_to_all(send_buf, msg_size);
            break;
        } case T_RESTART_GAME: {
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_RESTART_GAME, &NET_PROT_FMT_STR_STORAGE);
            send_to_all(send_buf, msg_size);
            break;
        } case T_GAME_STARTED_NO_JOINING: {
            int id;
            parse_msg_body(msg, &THREAD_PROT_FMT_STR_STORAGE, &id);
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_GAME_STARTED_NO_JOINING, &NET_PROT_FMT_STR_STORAGE);
            send_to_sock(id, send_buf, msg_size);
        }
        default:
            fprintf(stderr, "Invalid msg code in server_networking's process_thread_command() (%d)\n", code);
            break;
    }
}
