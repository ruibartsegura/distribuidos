#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

/*Global variables*/
#define SIZE 1024 // Number of characters for the input string

int main (int argc, char* argv[]) {
    DEBUG_PRINTF("argc %d\n", argc);
    if (argc != 4) {
        errno = EINVAL; // Invalid argument
        perror("Number of arguments incorrect");
        return 1;
    }

    // Match id, ip, port with the arguments
    int id = atoi(argv[1]);
    char* ip = argv[2];
    int port = atoi(argv[3]);

    setbuf(stdout, NULL); // Avoid buffering

    // Msg with the id of the client
    char msg_2_send[SIZE];
    snprintf(msg_2_send, sizeof(msg_2_send),
        "Hello server! From client: %d", id);
    char msg_2_rcv[SIZE];

    // Create socket
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("Error creating socket");
        return 1;
    }
    printf("Socket successfully created…\n");

    // Define and configure the server address structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert the IP address from text to binary form and validate it
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        close(socketfd);
        return EXIT_FAILURE;
    }

    // Connect socket
    // Connect socket
    if (connect(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("Error connecting");
        close(socketfd);
        return 1;
    }
    printf("connected to the server...\n");

    // Send msg
    if (send(socketfd, msg_2_send, strlen(msg_2_send), 0) == -1) {
        perror("Error sending msg");
        close(socketfd);
        return 1;
    }
    
    // Recive msg
    ssize_t bytes = recv(socketfd, msg_2_rcv, sizeof(msg_2_rcv) - 1, 0);
    if (bytes <= 0) {
        perror("Error receiving msg\n");
        close(socketfd);
        return 1;
    }
    printf("+++ %s\n", msg_2_rcv);

    // Free memory and close socket
    memset(msg_2_send, 0, sizeof(msg_2_send));
    memset(msg_2_rcv, 0, sizeof(msg_2_rcv));
    close(socketfd);
    return 0;
}

