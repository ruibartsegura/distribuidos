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
#include <pthread.h>
#include <getopt.h>
#include "stub.h"


#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

#define CONECCTIONS_LIMIT 600

void* thread_client(void *arg) {
    int sockfd = *((int *)arg);
    free(arg);

    reciver(sockfd);
    return NULL;
}

int main (int argc, char* argv[]) {
    // Get params
    static struct option long_options[] = {
        {"port",    required_argument, 0,  1},
        {"priority", required_argument, 0,  2},
        {0, 0, 0, 0}
    };
    while ((opt = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        switch (opt) {
            case 1: // ip
                thread_arg.ip = optarg;
                break;

            case 2: // port
                thread_arg.port = atoi(optarg);
                break;
        }
    }

    DEBUG_PRINTF("port %d\n", port);

    setbuf(stdout, NULL); // Avoid buffering
    
    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }
    printf("Socket successfully created...\n");

    // Define and configure the server address structure
    struct sockaddr_in server_addr, client;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    
    // Allow reusing the port for multiple connections
    const int enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding");
        return EXIT_FAILURE;
    }
    printf("Socket successfully binded...\n");

    // Listen
    if (listen(server_socket, 10000) == -1) {
        perror("Error listening");
        return EXIT_FAILURE;
    }
    printf("Server listening…\n");

    // Store the thread_fd to close it properly
    pthread_t thread_fd[CONECCTIONS_LIMIT];


    // Loop to control the conecctions
    while (true) {
        int count = 0;

        // Check if the maximum number of connections has been reached
        while (count < CONECCTIONS_LIMIT) {
            // Accept conecction
            socklen_t len = sizeof(client);
            
            int connfd = accept(server_socket, (struct sockaddr*)&client, &len);
            if (connfd == -1) {
                perror("Error accepting");
                close(server_socket);
                return EXIT_FAILURE;
            }

            // Allocate memory for connfd to avoid overwriting issues
            int *connfd_ptr = malloc(sizeof(int));
            *connfd_ptr = connfd;

            pthread_t thread;
            int status = pthread_create(&thread, NULL, &thread_client, connfd_ptr);

            // Check the correct creatin of the thread
            if (status != 0) {
                perror("pthread_create failed");
                close(server_socket);
                return EXIT_FAILURE;
            }
            
            thread_fd[count] = thread;
            count ++;
        }

        for (int x = 0; x < CONECCTIONS_LIMIT; x++) {
            if ( pthread_join( thread_fd[x], NULL) != 0) {
                perror("pthread_join failed");
                close(server_socket);
                return EXIT_FAILURE;
            }
        }
        count = 0; // Restart the counter
    }
}