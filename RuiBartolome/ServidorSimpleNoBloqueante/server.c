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

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

/*Global variables*/
#define PORT 8080
#define SIZE 1024 // Number of characters for the input string
bool EXIT_SIGNAL = false; // Bool to exit with control

// Handle CTRL C
void handle_sigint(int sig) {
    EXIT_SIGNAL = true;
}

/*
Serever Structure:
socket -> bind -> listen -> accept (se conecta client)-> 
recv -> send -> close
*/
int main (int argc, char* argv[]) {
    signal(SIGINT, handle_sigint); // Read CTRL C
    setbuf(stdout, NULL); // Avoid buffering

    int size = 1024; // Number of characters for the input string
    char msg_2_send[size];
    char msg_2_rcv[size];

    // Create socket
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("Error creating socket");
        return 1;
    }
    printf("Socket successfully created...\n");

    // Define and configure the server address structure
    struct sockaddr_in server_addr, client;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding");
        return 1;
    }
    printf("Socket successfully binded...\n");

    // Listen
    if (listen(socketfd, 1) == -1) {
        perror("Error listen");
        return 1;
    }
    printf("Server listening…\n");


    // Accept connection
    socklen_t len = sizeof(client);
    int connfd = accept(socketfd, (struct sockaddr*)&client, &len);

    // Start conversation, first recive then send
    while(!EXIT_SIGNAL) {
        fd_set readmask;
        struct timeval timeout;

        FD_ZERO(&readmask); // Initialize the file descriptor set to be empty
        FD_SET(socketfd, &readmask); // Add the server socket to the file descriptor set
        //timeout.tv_sec=0; timeout.tv_usec=500000; // Timeout 0.5 seg.

        // Select socket
        if (select(connfd+1, &readmask, NULL, NULL, &timeout)==-1)
            exit(-1);
        if (FD_ISSET(connfd, &readmask)){
            // Revive socket. Flag MSG_DONTWAIT makes it non-blocking 
            ssize_t bytes = recv(connfd, msg_2_rcv, sizeof(msg_2_rcv), MSG_DONTWAIT);
            if ((bytes == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)) { // Non blocking functionality
                continue;
            } else if ( bytes == -1) {
                perror("Error reciving msg");
                return 1;
            }
            printf("+++ %s\n", msg_2_rcv);
        }

        // Check if ctrl+C was executed
        if (EXIT_SIGNAL) break;

        // Get the input msg to send to the client
        printf("> ");
        fgets(msg_2_send, size, stdin);

        // Send msg
        if (send(connfd, msg_2_send, sizeof(msg_2_send), 0) == -1) {
            perror("Error sending msg");
            return 1;
        }

        // Clean the buffer
        memset(msg_2_send, 0, sizeof(msg_2_send));
        memset(msg_2_rcv, 0, sizeof(msg_2_rcv));
    }

    // Clean the buffer
    memset(msg_2_send, 0, sizeof(msg_2_send));
    memset(msg_2_rcv, 0, sizeof(msg_2_rcv));

    // Close sockets
    close(connfd);
    close(socketfd);
    return 0;
}
