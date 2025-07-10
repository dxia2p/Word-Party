#include "../public/network_header.h"
#include "../public/custom_protocol.h"
#include "../public/network_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

/* Function prototypes */
SOCKET connect_to_server(char *hostname, char *port);
//enum Recv_Status recv_from_server(SOCKET server_socket, char *msg, int msg_max_len);
//void send_to_server(SOCKET server_socket, char *msg, int msg_len);
void process_server_command(char *msg);
struct player_data *create_player(int id, const char *name);
void remove_player(int id);
struct player_data *get_player_from_id(int id);
void display_players();

struct player_data {
    char name[25];
    int id;
    struct player_data *next;
};

// Linked list of all players
static struct player_data *players;
bool game_started = false;
struct player_data *player_turn;

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
    sprintf(hostname, "127.0.0.1");
    sprintf(port, "8080");
    //printf("Enter hostname: ");
    //scanf("%99s", hostname);
    //getchar();
    //printf("Enter port: ");
    //scanf("%5s", port);
    //getchar();

    SOCKET socket_serv = connect_to_server(hostname, port);

    // Turn off nagles algorithm
    int yes = 1;
    if (setsockopt(socket_serv, IPPROTO_TCP, TCP_NODELAY, (void*)&yes, sizeof(yes)) < 0) {
        fprintf(stderr, "setsockopt() failed. (%d)\n", GETSOCKETERRNO());
    }
    
    struct receive_buffer *server_rb = create_receive_buffer();
    char recv_buffer[512];
    char send_buf[512];
    while (recv_from_sock(socket_serv, server_rb, recv_buffer, sizeof(recv_buffer)) == R_INCOMPLETE);

    if (get_msg_code(recv_buffer) == GAME_FULL){
        fprintf(stderr, "Game is full!\n");
        return 1;
    }
    printf("Joining game...\n");

    // At this point we should be successfully connected
    
    // Sending name to server
    char my_name[26];
    printf("Enter your name (max 24 char, no spaces): ");
    scanf("%24s", my_name);
    getchar();
    // check for invalid characters
    if (strchr(my_name, MSG_END) != NULL || strchr(my_name, MSG_VAL_SEP) != NULL) {
        fprintf(stderr, "You entered an invalid character!\n");
        return 1;
    }

    int msg_len = create_msg(send_buf, sizeof(send_buf), SEND_NAME, my_name);
    send_to_sock(socket_serv, send_buf, msg_len);
    printf("Submitted name to server!\n");


    // Multiplexing so that stdin and the server's socket doesn't block
    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_serv, &master);
    FD_SET(STDIN_FILENO, &master);
    SOCKET max_socket = socket_serv;
    if (STDIN_FILENO > max_socket) {
        max_socket = STDIN_FILENO;
    }

    while(1) {
        fd_set reads = master;
        if (select(max_socket + 1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        } 

        if (FD_ISSET(socket_serv, &master)) {  // server socket is ready for reading
            enum Recv_Status status = recv_from_sock(socket_serv, server_rb, recv_buffer, sizeof(recv_buffer));
            while (status != R_INCOMPLETE && status != R_DISCONNECTED) {  // Keep reading until there is nothing left to read
                process_server_command(recv_buffer);
                if (status == R_SINGLE_COMPLETE) {
                    break;
                }
                status = recv_from_sock(socket_serv, server_rb, recv_buffer, sizeof(recv_buffer));
            }
            if (status == R_DISCONNECTED) {
                return 1;
            }

            display_players();
        }

        if (FD_ISSET(STDIN_FILENO, &master)) {  // STDIN is ready for reading

        }
    }

    /* cleanup */
    delete_receive_buffer(server_rb);
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



/*
 
//This function assumes that server_socket is ready to be read from, it will block otherwise
//enum Recv_Status indicates if an incomplete message was received, one complete message was received, or more than one complete message was received

enum Recv_Status recv_from_server(SOCKET server_socket, char *msg, int msg_max_len) {     
    static char buffer[512];
    static int total_bytes = 0;

    // check if there is already a complete message in the buffer (in case we received 2 or more complete messages combined into 1
    if (is_message_complete(buffer, total_bytes)) { int len = get_msg_len(buffer); strncpy(msg, buffer, len); if (len != sizeof(buffer)) {
            memmove(buffer, buffer + len, sizeof(buffer) - len);  // TODO: CLEAR STUFF AFTER MOVING IT
            memset(buffer + (total_bytes - len), 0, sizeof(buffer) - (total_bytes - len));
        }
        total_bytes -= len;
        if (is_message_complete(buffer, total_bytes)) {
            return R_MULTIPLE_COMPLETE;
        }
        
        return R_SINGLE_COMPLETE;
    }
    
    int msg_bytes = recv(server_socket, buffer + total_bytes, sizeof(buffer) - total_bytes, 0);
    if (msg_bytes == 0) {
        fprintf(stderr, "Unexpected shutdown.\n");
        exit(1);
    }
    total_bytes += msg_bytes;


//printf("recv_from_server contents (%d bytes): %.*s\n", total_bytes, total_bytes, buffer);
    
    // Check if the message is complete
    if (is_message_complete(buffer, total_bytes)) {
        int len = get_msg_len(buffer);
        strncpy(msg, buffer, len);
        if (len != sizeof(buffer)) {
            memmove(buffer, buffer + len, sizeof(buffer) - len);
            memset(buffer + (total_bytes - len), 0, sizeof(buffer) - (total_bytes - len));
        }
        total_bytes -= len;
        if (is_message_complete(buffer, total_bytes)) {
            return R_MULTIPLE_COMPLETE;
        }
        
        return R_SINGLE_COMPLETE;
    }


    return R_INCOMPLETE;
}
*/

/*
void send_to_server(SOCKET server_socket, char *msg, int msg_len) {  
    int bytes_sent = 0;
    do {
        int msg_bytes = send(server_socket, msg + bytes_sent, msg_len - bytes_sent, 0);
        
        if (msg_bytes == -1) {
            fprintf(stderr, "send_to_server() failed. (%d)\n", GETSOCKETERRNO());
            exit(1);
        }
        bytes_sent += msg_bytes;

    } while (bytes_sent < msg_len);
}
*/


void process_server_command(char *msg) {
    char code = get_msg_code(msg);
    
    /*
    printf("Val count: %d\n", val_count);
    for (int i = 0; i < val_count; i++) {
        printf("msg_val: %s\n", separated_values[i]);
    }
    */

    switch(code) {
        case WORD_CORRECT: {
            break;
        }
        case WORD_INCORRECT: {
            break;
        }
        case LOSE_HP: {
            break;
        }
        case PLAYER_JOIN: { // id&name
            int id;
            char name[26];
            parse_msg_body(msg, &id, name);
            create_player(id, name);
            break;
        }            
        case PLAYER_LEFT: {  //id
            int id;
            parse_msg_body(msg, &id);
            remove_player(id);
            break;
        }
        case PLAYER_CHANGED_NAME: {  //id&name
            int id;
            char name[26];
            parse_msg_body(msg, &id, name);
            strcpy(get_player_from_id(id)->name, name);
            break;
        }
        default:
            printf("Invalid msg code (%d).\n", code);
            break;
    }
}


struct player_data *get_player_from_id(int id) {
    struct player_data *cur = players;
    while (cur != NULL) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    fprintf(stderr, "get_player_from_id() could not find player (%d!)\n", id);
    return NULL;
}

struct player_data *create_player(int id, const char *name) {
    struct player_data *temp = calloc(1, sizeof(struct player_data));
    if (temp == NULL) {
        fprintf(stderr, "calloc() in create_player() failed.\n");
        exit(1);
    }

    strcpy(temp->name, name);
    temp->id = id;

    // Add temp to the front of players linked list
    temp->next = players;
    players = temp;
    
    return temp;
}

void remove_player(int id) {
    struct player_data *cur = players;
    struct player_data *prev = NULL;
    while (cur->id != id && cur != NULL) {
        prev = cur;
        cur = cur->next;
    }

    if (cur == NULL) {
        fprintf(stderr, "remove_player() could not find player with id: %d\n", id);
        exit(1);
    }

    if (cur == players) {
        players = players->next;
    } else {
        prev->next = cur->next;
    }
    free(cur);
}

void display_players() {
    printf("%s", "\x1b[2J");  // Clear screen
    printf("\x1b[H");  // Move cursor to home
    printf("Players in the game are\n");
    printf("\x1b[4m");  // set underline mode

    struct player_data *cur = players;
    while (cur != NULL) {
        printf("%s\t", cur->name);
        cur = cur->next;
    }

    printf("\x1b[0m");
    printf("\n");
}
