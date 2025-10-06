#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

/*Global variables*/
#define PORT 8081
bool EXIT_SIGNAL = false; // Bool to exit with control


void handle_sigint(int sig) {
    EXIT_SIGNAL = true;
}

int main (int argc, char* argv[]) {
    signal(SIGINT, handle_sigint); // Read CTRL C
    setbuf(stdout, NULL); // Avoid buffering


    int size = 100; // Number of characters for the input string
    char msg_2_send[size];
    char msg_2_rcv[size];


    // Create socket and connect
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("Error creating socket");
        return 1;
    }
    
    printf("Socket successfully created...\n");
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding");
        return 1;
    }
    
    printf("Socket successfully binded...\n");
    
    
    if (listen(socketfd, 1) == -1) {
        perror("Error listening");
        return 1;
    }
    
    printf("Server listening…\n");

    while(!EXIT_SIGNAL) {
        if (recv(socketfd, msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
            perror("Error sending msg");
            return 1;
        }
        printf("+++ ");
        DEBUG_PRINTF("+++ %s\n", msg_2_send);
        
        if (EXIT_SIGNAL) break;
        
        printf("> ");
        fgets(msg_2_send, size, stdin); // Get the input msg to send to the server
        
        DEBUG_PRINTF("%s\n", msg_2_send);
        
        if (send(socketfd, msg_2_send, sizeof(msg_2_send), 0) == -1) {
            perror("Error sending msg");
            return 1;
        }
    }

    close(socketfd);
    return 0;
} 

/*
server_addr.sin_addr.s_addr = INADDR_ANY;

Socket con ip y puerto en el bind 

esperar cliente



 */