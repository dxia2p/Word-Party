#include "../include/network_header.h"

#include <stdio.h>
#include <string.h>

/* Function prototypes */
SOCKET connect_to_server(char *hostname, char *port);


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
    scanf("%s", hostname);
    getchar();
    printf("Enter port: ");
    scanf("%s", port);

    SOCKET socket_serv = connect_to_server(hostname, port);





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
