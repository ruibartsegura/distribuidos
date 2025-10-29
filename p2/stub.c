#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "stub.h"

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

int server_socket;
int client_socket;
unsigned int clock_lamport;


void* thread_client(void *arg) {
    int sockfd = *((int *)arg);
    free(arg);

    struct message msg_2_rcv;

    if (recv(sockfd, &msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
        perror("Error reciving msg");
        close(sockfd);
        return NULL;
    }
}

int init_coordinator (const int port) {
    setbuf(stdout, NULL); // Avoid buffering

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        return 1;
    }
    printf("Socket successfully created...\n");
    
    // Bind socket
    struct sockaddr_in server_addr, client;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    
    const int enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        return EXIT_FAILURE;
    }

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding");
        return EXIT_FAILURE;
    }
    printf("Socket successfully binded...\n");
    
    // Listen
    if (listen(server_socket, 1) == -1) {
        perror("Error listening");
        return EXIT_FAILURE;
    }
    printf("Server listening…\n");

    socklen_t len = sizeof(client);
    client_socket = accept(server_socket, (struct sockaddr*)&client, &len);
    
    pthread_t thread;
    pthread_create(&thread, NULL, &thread_client, &client_socket);
    pthread_detach(thread);
    
    return EXIT_SUCCESS;
}

int connect_2_server(const char* ip, const int port) {
    setbuf(stdout, NULL); // Avoid buffering

    // Create socket and connect
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }
    printf("Socket successfully created…\n");
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        close(socketfd);
        return EXIT_FAILURE;
    }

    if (connect(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("Error connecting");
        close(socketfd);
        return EXIT_FAILURE;
    }
    printf("connected to the server...\n");
}

void ready_2_shutdown(const char origin[20], const int lamport_count){
     struct message msg_2_send;
     msg_2_send.origin = origin;
     msg_2_send.action = READY_TO_SHUTDOWN;
     msg_2_send.clock_lamport = lamport_count;

     if (send(socketfd, &msg_2_send, strlen(msg_2_send), 0) == -1) {
        perror("Error sending msg");
        close(socketfd);
        return EXIT_FAILURE;
    }
    printf("%s, %d, SEND, READY_TO_SHUTDOWN", origin, lamport_count);
    return EXIT_SUCCESS;
}

void shutdown_confirm(const char origin[20], const int lamport_count) {
    struct message msg_2_send;
    msg_2_send.origin = origin;
    msg_2_send.action = SHUTDOWN_NOW;
    msg_2_send.clock_lamport = lamport_count;

    if (send(socketfd, &msg_2_send, strlen(msg_2_send), 0) == -1) {
        perror("Error sending msg");
        close(socketfd);
        return EXIT_FAILURE;
    }
    printf("%s, %d, SEND, SHUTDOWN_NOW", origin, lamport_count);
    return EXIT_SUCCESS;
}

void shutdown_done(const char origin[20], const int lamport_count) {
    struct message msg_2_send;
    msg_2_send.origin = origin;
    msg_2_send.action = SHUTDOWN_ACK;
    msg_2_send.clock_lamport = lamport_count;

    if (send(socketfd, &msg_2_send, strlen(msg_2_send), 0) == -1) {
        perror("Error sending msg");
        close(socketfd);
        return EXIT_FAILURE;
    }
    printf("%s, %d, SEND, SHUTDOWN_ACK", origin, lamport_count);
    return EXIT_SUCCESS;
}

int get_clock_lamport() {
    return clock_lamport;
}
