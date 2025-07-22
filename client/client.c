#include "../public/network_header.h"
#include "../public/custom_protocol.h"
#include "../public/network_handler.h"
#include "../public/network_protocol_codes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>

static const int MAX_HP = 3;

struct player_data {
    char name[25];
    int id;
    int hp;
    struct player_data *next;
};
struct alert {
    char str[128];
    double time_left;
    struct alert *next;
};

/* Function prototypes */
void update_alerts(double deltatime);  // Updates and draws alerts
void create_alert(char *msg, double time_to_live);
void destroy_alert(struct alert *a, struct alert *prev);
void enable_raw_input();
void disable_raw_input();
void process_terminal_input(char *inp, int len, SOCKET server_sock);
SOCKET connect_to_server(char *hostname, char *port);
void process_server_command(char *msg);
struct player_data *create_player(int id, const char *name);
void remove_player(int id);
struct player_data *get_player_from_id(int id);
void display_ui(char *input);

/* Variables */
struct termios original_termios;
static struct player_data *players = NULL;  // Linked list of all players
static int my_id;
static bool game_started = false;
static struct player_data *player_turn = NULL;
static char req_substr[4] = {'\0'};
static double time_left = -1.0;
static struct alert *alerts = NULL;

int main(int argc, char *argv[]) {
    #if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
    #endif

    char hostname[100];
    char port[6];

    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        sprintf(hostname, "127.0.0.1");
        sprintf(port, "8080"); 
    } else {
        printf("Enter hostname: ");
        scanf("%99s", hostname);
        getchar();
        printf("Enter port: ");
        scanf("%5s", port);
        getchar();
    }
    
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

    if (get_msg_code(recv_buffer) == N_GAME_FULL){
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

    int msg_len = create_msg(send_buf, sizeof(send_buf), N_SEND_NAME, &NET_PROT_FMT_STR_STORAGE, my_name);
    send_to_sock(socket_serv, send_buf, msg_len);
    printf("Submitted name to server!\n");

    /* Multiplexing */
    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_serv, &master);
    FD_SET(STDIN_FILENO, &master);
    SOCKET max_socket = socket_serv;
    if (STDIN_FILENO > max_socket) {
        max_socket = STDIN_FILENO;
    }

    /* Terminal input */
    char input[128] = {'\0'};
    int input_len = 0;
    enable_raw_input();

    /* frametime */
    struct timespec prev_time;
    clock_gettime(CLOCK_MONOTONIC, &prev_time);
    double deltatime = 0;

    while(1) {
        fd_set reads = master;
        if (select(max_socket + 1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        } 

        if (FD_ISSET(socket_serv, &reads)) {  // server socket is ready for reading
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

        }

        if (FD_ISSET(STDIN_FILENO, &reads) && game_started) {  // STDIN is ready for reading
            char c;
            read(STDIN_FILENO, &c, 1);
            input[input_len] = c;
            input_len++;
            input[input_len] = '\0';

            if (input[input_len - 1] == '\n') {
                process_terminal_input(input, input_len, socket_serv);
                input_len = 0;
                input[0] = '\0';
            } else if (input[input_len - 1] == '\b') {
                input_len -= 2;
                input[input_len] = '\0';
            } else if (input_len >= sizeof(input) - 1) {
                input_len = 0;
                input[0] = '\0';
            }
        }

        /* user interface */
        display_ui(input);
        update_alerts(deltatime);

        /* frametime */
        struct timespec cur_time;
        clock_gettime(CLOCK_MONOTONIC, &cur_time);
        struct timespec frame_time;
        frame_time.tv_sec = cur_time.tv_sec - prev_time.tv_sec;
        frame_time.tv_nsec = cur_time.tv_nsec - prev_time.tv_nsec;
        if (frame_time.tv_sec > 0 && frame_time.tv_nsec < 0) {
            frame_time.tv_nsec += 1e9;
            frame_time.tv_sec--;
        }
        deltatime = frame_time.tv_sec;
        deltatime += ((double)frame_time.tv_nsec) / 1e9;
        prev_time = cur_time;
        // printf("frame_time.tv_sec: %ld, frame_time.tv_nsec: %ld, deltatime %f\n", frame_time.tv_sec, frame_time.tv_nsec, deltatime);
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

void update_alerts(double deltatime) {
    printf("\n");
    struct alert *cur = alerts;
    struct alert *prev = NULL;
    while (cur != NULL) {
        cur->time_left -= deltatime;
        if (cur->time_left <= 0) {
            destroy_alert(cur, prev);
            cur = cur->next;
            continue;
        }
        
        printf("\x1b[3m%s\n\x1b[0m", cur->str);

        prev = cur;
        cur = cur->next;
    }
}

void create_alert(char *msg, double time_to_live) {
    struct alert *temp = malloc(sizeof(struct alert));
    if (temp == NULL) {
        fprintf(stderr, "Failed to malloc() in create_alert()\n");
        return;
    }
    strcpy(temp->str, msg);
    temp->time_left = time_to_live;
    if (alerts == NULL) {
        alerts = temp;
    }
    else {
        temp->next = alerts;
        alerts = temp;
    }
}

void destroy_alert(struct alert *a, struct alert *prev) {
    if (prev == NULL) {
        alerts = alerts->next;
    } else {
        prev->next = a->next;
    }

    free(a);
}

void enable_raw_input() {
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(disable_raw_input);

    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);  // ICANON isn't an input flag for some reason
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_input() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

// This function just sends inp to server for now
// Might add more commands later
void process_terminal_input(char *inp, int len, SOCKET server_sock) {
    inp[len - 1] = '\0';  // Remove newline
    static char msg_buf[128];
    int msg_size = create_msg(msg_buf, sizeof(msg_buf), N_SEND_WORD, &NET_PROT_FMT_STR_STORAGE, inp);
    send_to_sock(server_sock, msg_buf, msg_size);
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

struct player_data *get_player_from_id(int id) {
    struct player_data *cur = players;
    while (cur != NULL) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    fprintf(stderr, "get_player_from_id() could not find player (%d)!\n", id);
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
    temp->hp = MAX_HP;

    // Add temp to the end of players linked list to keep order consistent
    if (players != NULL) {
        struct player_data *cur = players;
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = temp;
    } else {
        players = temp;
    }
   
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

void display_ui(char *input) {
    printf("\x1b[2J\x1b[3J");  // Clear screen
    printf("\x1b[H");  // Move cursor to home
    
    printf("+---------------------------------------------------------------------------+\n");

    if (game_started) {
        printf("The game has started!\n\n");
    }

    printf("Players in the game are\n");

    struct player_data *cur = players;
    while (cur != NULL) {
        if (cur->hp <= 0) {
            printf("\x1b[2m");
        } else {
            printf("\x1b[4m");
        }
        if (cur == player_turn) {
            printf("\x1b[1;36m");
        }
        
        printf("%s\x1b[0m (hp: %d)\t", cur->name, cur->hp);
        
        cur = cur->next;
    }

    if (game_started) {
        printf("\n\n");
        printf("It's \x1b[1;36m%s\x1b[0m's turn.\n\n", player_turn->name);
        printf("Their word must contain \x1b[32;1m[%s]\x1b[0m.\n\n", req_substr);
        if(player_turn->id == my_id) {
            printf("You have \x1b[31;1m%.2lf\x1b[0m seconds left!\n", time_left);
        } else {
            printf("They have \x1b[31;1m%.2lf\x1b[0m seconds left!\n", time_left);
        }
    }

    printf("\n+---------------------------------------------------------------------------+\n");

    if (player_turn != NULL && game_started && player_turn->id == my_id) {
        printf("It's your turn! Enter a word containing \x1b[32;1m[%s]\x1b[0m: ", req_substr);
    }

    printf("%s", input);
    fflush(stdout);

    printf("\x1b[0m");
}

void process_server_command(char *msg) {
    static const double default_alert_ttl = 3.5;

    char code = get_msg_code(msg);
    
    /*
    printf("Val count: %d\n", val_count);
    for (int i = 0; i < val_count; i++) {
        printf("msg_val: %s\n", separated_values[i]);
    }
    */

    static char temp_buf[128];
    static char recv_buf[128];

    switch(code) {
        case N_SEND_REQ_STR: {
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, req_substr);
            break;
        }
        case N_CORRECT_WORD: {
            int id;
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, &id, recv_buf);
            if (id == my_id) {
                snprintf(temp_buf, sizeof(temp_buf), "You entered a correct word! (%s)", recv_buf);
                create_alert(temp_buf, default_alert_ttl);
            } else {
                snprintf(temp_buf, sizeof(temp_buf), "%s entered %s, which is correct!", get_player_from_id(id)->name, recv_buf);
                create_alert(temp_buf, default_alert_ttl);
            }
            break;
        }
        case N_INCORRECT_WORD: {
            int id;
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, &id);
            if (id == my_id) {
                create_alert("Invalid word!", default_alert_ttl);
            }
            break;
        }
        case N_PLAYER_JOIN: { // id&name
            int id;
            char name[26];
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, &id, name);
            printf("id: %d\n", id);
            create_player(id, name);
            break;
        }            
        case N_PLAYER_LEFT: {  //id
            int id;
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, &id);
            remove_player(id);
            break;
        }
        case N_PLAYER_CHANGED_NAME: {  //id&name
            int id;
            char name[26];
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, &id, name);
            strcpy(get_player_from_id(id)->name, name);
            break;
        }
        case N_GAME_START: {
            game_started = true;
            break;
        }
        case N_TURN_TIME: {
            double t;
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, &t);
            time_left = t;
            break;
        } case N_PLAYER_TURN: {
            int id;
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, &id);
            player_turn = get_player_from_id(id);
            break;
        } case N_SEND_ID: {
            int id;
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, &id);
            my_id = id;
            break;
        } case N_LOSE_HP: {
            int id;
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, &id);
            struct player_data *p = get_player_from_id(id);
            p->hp--;
            snprintf(temp_buf, sizeof(temp_buf), "%s has lost one hp!", p->name);
            create_alert(temp_buf, default_alert_ttl);
            break;
        } case N_PLAYER_WON: {
            int id;
            parse_msg_body(msg, &NET_PROT_FMT_STR_STORAGE, &id);
            struct player_data *p = get_player_from_id(id);
            snprintf(temp_buf, sizeof(temp_buf), "THE WINNER IS %s!!!", p->name);
            create_alert(temp_buf, 10.0);
        }
        default:
            fprintf(stderr, "Invalid msg code in process_server_command() (%d).\n", code);
            break;
    }
}


