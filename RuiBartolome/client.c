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
        
        printf("%s\n", msg_2_send);
        
        if (send(socketfd, msg_2_send, sizeof(msg_2_send), 0) == -1) {
            perror("Error sending msg");
            return 1;
        }
        
        if (EXIT_SIGNAL) break;
        
        if (recv(socketfd, msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
            perror("Error sending msg");
            return 1;
        }
        printf("+++ %s\n", msg_2_send);
    }

    close(socketfd);
    return 0;
} 
    
/* TODO
Crtl C cambia variable para salir de forma controlado

Uno lee y otro escribe cuidado en el codigo
Cliente escribe primero

MSG_DONTWAIT flag no bloqueante


Conect genera IP y puerto 
*/ 