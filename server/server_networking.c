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
    //FD_SET(STDIN_FILENO, &master);
    /*
    if (STDIN_FILENO > max_socket) {
        max_socket = STDIN_FILENO;
    }
    */

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
            // HANDLE THINGS IF PLAYER COUNT IS FULL
            if (client_count == MAX_CLIENT_COUNT) {
                printf("Reached max player count. Dropping player (%d)...\n", client_socket);
                int msg_size = create_msg(send_buf, sizeof(send_buf), N_GAME_FULL, net_protocol_fmtstrs, sizeof(net_protocol_fmtstrs));
                send_to_sock(client_socket, send_buf, msg_size);
                CLOSESOCKET(client_socket);
                continue;
            }
            int msg_size = create_msg(send_buf, sizeof(send_buf), N_GAME_NOT_FULL, net_protocol_fmtstrs, sizeof(net_protocol_fmtstrs));

            FD_SET(client_socket, &master);
            if(client_socket > max_socket) {
                max_socket = client_socket;
            }

            create_client_data(client_socket);
            client_count++;
            printf("NEW CLIENT HAS JOINED!!!!\n");
            msg_size = create_msg(send_buf, sizeof(send_buf), T_PLAYER_JOIN, thread_protocol_fmtstrs, sizeof(thread_protocol_fmtstrs), client_socket, "Unnamed");
            message_enqueue(write_queue, send_buf, msg_size);
            msg_size = create_msg(send_buf, sizeof(send_buf), N_PLAYER_JOIN, net_protocol_fmtstrs, sizeof(net_protocol_fmtstrs), client_socket, "Unnamed");
            send_to_all(send_buf, msg_size);

            // Send the names of all existing players to the new player 
            struct client_data *cur = clients;
            while (cur != NULL) {
                if (cur->sock == client_socket) {  // Don't send the new client's name to themselves
                    cur = cur->next;
                    continue;
                }  
                msg_size = create_msg(send_buf, sizeof(send_buf), N_PLAYER_JOIN, net_protocol_fmtstrs, sizeof(net_protocol_fmtstrs), cur->sock, "Unnamed");
                //print_msg(msg_buf);
                send_to_sock(client_socket, send_buf, msg_size);
                cur = cur->next;
            }
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
                    int msg_size = create_msg(send_buf, sizeof(send_buf), N_PLAYER_LEFT, net_protocol_fmtstrs, sizeof(net_protocol_fmtstrs), cur->sock);
                    send_to_all_except_one(send_buf, msg_size, cur->sock);
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
    temp->next = clients;  // Add temp to client list
    clients = temp;
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
            parse_msg_body(msg, net_protocol_fmtstrs, sizeof(net_protocol_fmtstrs), recv_str);
            int msg_size = create_msg(send_buf, sizeof(send_buf), T_SEND_NAME, thread_protocol_fmtstrs, sizeof(thread_protocol_fmtstrs), cd->sock, recv_str);
            message_enqueue(write_queue, send_buf, msg_size);
            break;
        }
        default:
            // Do something
            break;
    }
}

static void process_thread_command(char *msg) {
    char code = get_msg_code(msg);
    int len = get_msg_len(msg);
    static char temp_buf[256];

    switch (code) {
        default:
            break;
    }
}
