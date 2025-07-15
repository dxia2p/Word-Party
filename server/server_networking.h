#ifndef SERVER_NETWORKING_H
#define SERVER_NETWORKING_H

#include "../public/network_header.h"

void* networking_main_routine(void *queue_group);
SOCKET create_server(char *hostname, char *port);
SOCKET wait_for_client(SOCKET socket_listen);
struct client_data *create_client_data(SOCKET s);
void send_to_all(char *msg, int msg_len);
void send_to_all_except_one(char *msg, int msg_len, SOCKET except_this_socket);

#endif
