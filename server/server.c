#include "../include/network_header.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function prototypes
SOCKET create_server(char *hostname, char *port);


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

    printf("Main loop is starting...\n");

    while (1) {
        fd_set reads = master;  // Make copies of master
        fd_set writes = master;

        if (select(max_socket, &reads, &writes, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }


        // Loop through each socket to see if it was flagged by select()
        // Not the best for performance but it shouldn't be a bottleneck
        SOCKET i;
        for (i = 1; i <= max_socket; ++i) {
            if (FD_ISSET(i, &reads)) {
                if (i == socket_listen) {
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    SOCKET socket_client = accept(socket_listen, (struct sockaddr*) &client_address, &client_len);

                    if(!ISVALIDSOCKET(socket_client)) {
                        fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
                        return 1;
                    }
                    
                    FD_SET(socket_client, &master);
                    if (socket_client > max_socket)
                        max_socket = socket_client;
                    char address_buffer[100];
                    getnameinfo((struct sockaddr*)&client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
                    printf("%s has connected.\n", address_buffer);

                } else {

                }
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
