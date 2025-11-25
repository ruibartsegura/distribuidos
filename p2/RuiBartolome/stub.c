#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "stub.h"

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

#define CLIENTS 2 // Number of clients

char id[20]; // Will keep the name of the proccess

int server_socket = -1;
int server_sockets_fd[CLIENTS] = {-1}; // Will have the fds to comm server -> clients
char server_sockets_name[CLIENTS][20] = {0}; // Will have the name of the proccess to comm server -> clients

int client_counter = 0; // Counts how many clients we have already add to the list

unsigned int clock_lamport = 0;

bool shutdown_complete = false;

int thread_count = 0;
pthread_t thread_fd[CLIENTS+1] = {-1}; // Save the fd of the trhead to close everything at the end

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* Increase the lamport clock */
void increment_lamport() {
    pthread_mutex_lock(&lock);
    clock_lamport++;
    pthread_mutex_unlock(&lock);
}

char* get_action_name(int act_id){
    switch (act_id) {
        case READY_TO_SHUTDOWN:
            return "READY_TO_SHUTDOWN";
        case SHUTDOWN_NOW:
            return "SHUTDOWN_NOW";
        case SHUTDOWN_ACK:
            return "SHUTDOWN_ACK";
        default: 
            return "UNKNOWN";
    }
}

/* Thread where the sockets do recv */
void* thread_reciver(void *arg) {
    int sockfd = *((int *)arg);
    free(arg);

    // RECV
    struct message msg_2_rcv;
    if (recv(sockfd, &msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
        perror("Error reciving msg");
        close(sockfd);
        return NULL;
    }

    // Check if the in comming fd is stored and matched with the identifier name
    bool already_exist = false; 
    for (int x = 0; x < CLIENTS; x++) {
        if (strcmp(server_sockets_name[x], msg_2_rcv.origin) == 0) {
            // If the name is already stored we can skip the change name part
            already_exist = true;
            continue;
        }
        if (server_sockets_fd[x] == sockfd) {
            // If the fd already exists but has a different name, we update it.
            // This happens because when the server first creates the client fd,
            // it doesn’t yet know the client's name, so it’s initially assigned
            // the server process name by default. Or the order of incoming msg is wrong
            strcpy(server_sockets_name[x], msg_2_rcv.origin);
            continue;
        }
    }

    // Add the new client to the list if needed
    if (already_exist == false) {
        pthread_mutex_lock(&lock);
        server_sockets_fd[client_counter] = sockfd;
        strcpy(server_sockets_name[client_counter], msg_2_rcv.origin);
        client_counter++;
        pthread_mutex_unlock(&lock);
    }

    // Check which clock is bigger and take it as the new one
    pthread_mutex_lock(&lock);
    if (msg_2_rcv.clock_lamport > clock_lamport) {
        clock_lamport = msg_2_rcv.clock_lamport;
    }
    pthread_mutex_unlock(&lock);

    increment_lamport();

    // Print the msg of recived
    printf("%s, %d, RECV (%s), %s\n", id, clock_lamport, msg_2_rcv.origin, get_action_name(msg_2_rcv.action));
    memset(&msg_2_rcv, 0, sizeof(msg_2_rcv));
    return NULL;
}

/* Py start lsitening Px -> Create the thread to do the recv */
int listen_to(const char px_name[20]) {
    // Match the procces to recive the msg with the input name 
    int socket_fd = -1;
    for (int x = 0; x < CLIENTS; x++) {
        if (strcmp(server_sockets_name[x], px_name) == 0)
            socket_fd = server_sockets_fd[x];
    }

    // Check that the procces name is a actual socket
    if (socket_fd == -1) {
        perror("No procces with that name");
        return EXIT_FAILURE;
    } 

    int *socket_fd_ptr = malloc(sizeof(int));
    *socket_fd_ptr = socket_fd;

    pthread_t thread;
    int status = pthread_create(&thread, NULL, &thread_reciver, socket_fd_ptr);

    // Check the correct creatin of the thread
    if (status != 0) {
        perror("pthread_create failed");
        return EXIT_FAILURE;
    }

    thread_fd[thread_count] = thread; // Add the thread fd to the list
    thread_count++;
    return EXIT_SUCCESS;
}

/* Px establish connection with Py -> Server accept connection */
int establish_connection(const char px_name[20]) {
    struct sockaddr_in client;

    socklen_t len = sizeof(client);
    int connfd = accept(server_socket, (struct sockaddr*)&client, &len);

    // Save the fd & name of the socket to further interactions
    server_sockets_fd[client_counter] = connfd;
    strcpy(server_sockets_name[client_counter], px_name);
    client_counter++;
    return EXIT_SUCCESS;
}

/* Initialize the server */
int init_coordinator (const char* ip, const int port, const char init_id[20]) {
    strcpy(id, init_id); // From now it will use this id.

    setbuf(stdout, NULL); // Avoid buffering

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        return 1;
    }

    // Bind socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert the IP address from text to binary form and validate it
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        close(server_socket);
        return EXIT_FAILURE;
    }

    const int enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        return EXIT_FAILURE;
    }

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding");
        return EXIT_FAILURE;
    }

    // Listen
    if (listen(server_socket, 1) == -1) {
        perror("Error listening");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/* Initialize the client */
int join_network(const char* ip, const int port, const char proc_id[20], const char hoster_id[20]) {
    strcpy(id, proc_id); // From now it will use this id.
    
    setbuf(stdout, NULL); // Avoid buffering

    // Create socket and connect
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        close(client_socket);
        return EXIT_FAILURE;
    }

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("Error connecting");
        close(client_socket);
        return EXIT_FAILURE;
    }

    // Add the socket to the list of them
    server_sockets_fd[0] = client_socket;
    strcpy(server_sockets_name[0], hoster_id);

    return EXIT_SUCCESS;
}

/* Send a message (op) to another proccess */
int communicate(const char to_who[20], enum operations op) {
    increment_lamport();

    // Check if the destination socket exist
    int socket_fd;
    for (int x = 0; x < CLIENTS; x++) {
        if (strcmp(to_who, server_sockets_name[x]) == 0) {
            socket_fd = server_sockets_fd[x];
            break;
        }
    }

    if (socket_fd == -1) {
        perror("Wrong destination name");
        return EXIT_FAILURE;
    }

    struct message msg_2_send;
    memset(&msg_2_send, 0, sizeof(msg_2_send));

    // Add the info to send
    strcpy(msg_2_send.origin, id);
    msg_2_send.action = op;
    msg_2_send.clock_lamport = clock_lamport;

    // SEND
    if (send(socket_fd, &msg_2_send, sizeof(msg_2_send), 0) == -1) {
        perror("Error sending msg");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    memset(&msg_2_send, 0, sizeof(msg_2_send));
    printf("%s, %d, SEND, %s\n", id, clock_lamport, get_action_name(op));
    return EXIT_SUCCESS;
}

/* Function to get the actual value of the lamport clock */
unsigned int get_clock_lamport() {
    pthread_mutex_lock(&lock);
    unsigned int value = clock_lamport;
    pthread_mutex_unlock(&lock);
    return value;
}

/* Once the shutdown is confirm close sockets, threads, ... */
void close_everything() {
    shutdown_complete = true;
    
    // Wait untill the threads finish
    for (int x = 0; x < thread_count ; x++) {
        if (thread_fd[x] != -1)
            pthread_join( thread_fd[x], NULL);
    }
    
    // Close the sockets
    for (int x = 0; x < CLIENTS ; x++) {
        if (server_sockets_fd[x] != -1)
            close(server_sockets_fd[x]);
    }

    // If exist close the server socket and print the last msg wuth the value of lamport clock
    if (server_socket != -1) {
        close(server_socket);
        printf("Los clientes fueron correctamente apagados en t(lamport) = %d\n", clock_lamport);
    }
}

