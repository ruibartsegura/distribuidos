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

bool EXIT_SIGNAL = false; // Bool to exit with control

void handle_sigint(int sig) {
    EXIT_SIGNAL = true;
}

int main (int argc, char* argv[]) {
    signal(SIGINT, handle_sigint); // Read CTRL C

    struct sockaddr_in servaddr;

    int size = 100; // Number of characters for the input string
    char msg_2_send[size];
    char msg_2_rcv[size];

    // Create socket and connect
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("Error creating socket");
        return 1;
    }

    if (connect(socketfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) {
        perror("Error connecting");
        return 1;
    }


    // Start of the conversation
    // Read de stdin
    // Print any msg recived from the server

    while(!EXIT_SIGNAL) {
        printf("> ");
        fgets(msg_2_send, size, stdin); // Get the input msg to send to the server

        DEBUG_PRINTF("%s\n", msg_2_send);

        if (send(socketfd, msg_2_send, sizeof(msg_2_send), 0) == -1) {
            perror("Error sending msg");
            return 1;
        }
        
        if (EXIT_SIGNAL) break;
        
        if (recv(socketfd, msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
            perror("Error sending msg");
            return 1;
        }
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