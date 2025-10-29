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
#define PORT 8080
#define SIZE 1024 // Number of characters for the input string
bool EXIT_SIGNAL = false; // Bool to exit with control

void handle_sigint(int sig) {
    EXIT_SIGNAL = true;
}

int main (int argc, char* argv[]) {
    signal(SIGINT, handle_sigint); // Read CTRL C
    setbuf(stdout, NULL); // Avoid buffering

    char msg_2_send[SIZE];
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
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // Connect socket
    if (connect(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("Error connecting");
        return 1;
    }
    printf("connected to the server...\n");

    // Start conversation, first send then recive
    while(!EXIT_SIGNAL) {
        printf("> ");
        fgets(msg_2_send, SIZE, stdin); // Get the input msg to send to the server

        // Send msg
        if (send(socketfd, msg_2_send, sizeof(msg_2_send), 0) == -1) {
            perror("Error sending msg");
            return 1;
        }

        // Check if ctrl+C was executed
        if (EXIT_SIGNAL) break;

        // Recive msg
        if (recv(socketfd, msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
            perror("Error sending msg");
            return 1;
        }
        printf("+++ %s\n", msg_2_rcv);

        // Clean the buffer
        memset(msg_2_send, 0, sizeof(msg_2_send));
    }

    // Clean the buffer
    memset(msg_2_send, 0, sizeof(msg_2_send));
    memset(msg_2_rcv, 0, sizeof(msg_2_rcv));

    // Close socket
    close(socketfd);
    return 0;
}
