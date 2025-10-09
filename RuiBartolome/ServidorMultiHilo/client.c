#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>
#include <stdlib.h>

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

/*Global variables*/
#define PORT 8080
bool EXIT_SIGNAL = false; // Bool to exit with control

void handle_sigint(int sig) {
    EXIT_SIGNAL = true;
}

int main (int argc, char* argv[]) {
    // Adjust the argument count and pointer to skip the program name
    argc -= 1;
    argv += 1;

    if (argc != 2) {
        errno = EINVAL; // Invalid argument
        perror("Number of arguments incorrect");
        return 1;
    }

    char* ip = argv[0]; 
    char* port = argv[1]; 
    
    DEBUG_PRINTF("ip %s\n", ip);
    DEBUG_PRINTF("port %s\n", port);
    
    signal(SIGINT, handle_sigint); // Read CTRL C
    setbuf(stdout, NULL); // Avoid buffering

    int size = 1024; // Number of characters for the input string
    char msg_2_send[size];
    char msg_2_rcv[size];

    // Create socket and connect
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("Error creating socket");
        return 1;
    }
    
    printf("Socket successfully created…\n");
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    
    if (connect(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("Error connecting");
        return 1;
    }
    
    printf("connected to the server...\n");

    // Start of the conversation
    // Read de stdin
    // Print any msg recived from the server

    while(!EXIT_SIGNAL) {
        printf("> ");
        fgets(msg_2_send, size, stdin); // Get the input msg to send to the server
        
        if (send(socketfd, msg_2_send, sizeof(msg_2_send), 0) == -1) {
            perror("Error sending msg");
            return 1;
        }
        
        if (EXIT_SIGNAL) break;

        fd_set readmask;
        struct timeval timeout;

        FD_ZERO(&readmask); // Reset conjunto de descriptores
        FD_SET(socketfd, &readmask); // Asignamos el nuevo descriptor
        timeout.tv_sec=0; timeout.tv_usec=500000; // Timeout de 0.5 seg.

        if (select(socketfd+1, &readmask, NULL, NULL, &timeout)==-1)
            exit(-1);
        if (FD_ISSET(socketfd, &readmask)){
            // Flag MSG_DONTWAIT makes it non-blocking 
            ssize_t bytes = recv(socketfd, msg_2_rcv, sizeof(msg_2_rcv), MSG_DONTWAIT);
            if ((bytes == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)) { // Non blocking functionality
                continue;
            } else if ( bytes == -1) {
                perror("Error reciving msg");
                return 1;
            }
            printf("+++ %s\n", msg_2_rcv);
        }
        

        // Clean the buffer
        memset(msg_2_send, 0, sizeof(msg_2_send));
        memset(msg_2_rcv, 0, sizeof(msg_2_rcv));
    }
    
    memset(msg_2_send, 0, sizeof(msg_2_send));
    memset(msg_2_rcv, 0, sizeof(msg_2_rcv));
    close(socketfd);
    return 0;
} 
