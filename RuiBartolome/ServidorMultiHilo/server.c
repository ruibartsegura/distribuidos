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
#include <time.h>
#include <stdatomic.h>

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

/*Global variables*/
#define SIZE 1024 // Number of characters for the input string
#define CONECCTIONS_LIMIT 100 // Number of conecctionss allowed
atomic_int NUM_THREADS = 0; // Number of current threads
bool EXIT_SIGNAL = false; // Control the ctrl+C

// Handle CTRL C
void handle_sigint(int sig) {
    EXIT_SIGNAL = true;
}

// Function to generate the random wait of the server (0.5seg-2.0seg)
float random_generator(unsigned int *seed) {
    float min = 0.5, max = 2.0;
    float num;
    num = min + ((float)rand_r(seed) / RAND_MAX) * (max - min);
    DEBUG_PRINTF("Random = %f", num);
    return num;
}

// Function to treat each client in a thread
void* thread_client(void *arg) {
    atomic_fetch_add(&NUM_THREADS, 1);
    int sockfd = *((int *)arg);
    free(arg);

    char msg_2_send[SIZE] = "Hello client";
    char msg_2_rcv[SIZE];

    // Recivve msg
    if (recv(sockfd, msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
        perror("Error reciving msg");
        close(sockfd);
        atomic_fetch_sub(&NUM_THREADS, 1);
        return NULL;
    }

    // Get a seed for random thread safe
    unsigned int seed = time(NULL) ^ pthread_self();
    float num = random_generator(&seed); // Random value between 0.5 - 2.0
    
    // Sleep the random period
    usleep((useconds_t)(num * 1000000));
    
    // Print recived msg
    printf("+++ %s\n", msg_2_rcv);

    // Send hello client
    if (send(sockfd, msg_2_send, sizeof(msg_2_send), 0) == -1) {
        perror("Error sending msg");
        close(sockfd);
        atomic_fetch_sub(&NUM_THREADS, 1);
        return NULL;
    }

    // Clean the buffer
    memset(msg_2_send, 0, sizeof(msg_2_send));
    memset(msg_2_rcv, 0, sizeof(msg_2_rcv));

    // Close socket and -1 num of threads
    close(sockfd);
    atomic_fetch_sub(&NUM_THREADS, 1);
    return NULL;
}

/*
Serever Structure:
socket -> bind -> listen -> accept (se conecta client)->
create thread -> recv -> send -> close
*/
int main (int argc, char* argv[]) {
    // Adjust the argument count and pointer to skip the program name
    argc -= 1;
    argv += 1;

    if (argc != 1) {
        errno = EINVAL; // Invalid argument
        perror("Number of arguments incorrect");
        return 1;
    }

    int port = atoi(argv[0]);
    DEBUG_PRINTF("port %d\n", port);

    signal(SIGINT, handle_sigint); // Handle CTRL C
    setbuf(stdout, NULL); // Avoid buffering
    
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
    server_addr.sin_port = htons(port);
    
    // Allow reusing the port for multiple connections
    const int enable = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

    // Bind socket
    if (bind(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding");
        return 1;
    }
    printf("Socket successfully binded...\n");

    // Listen
    if (listen(socketfd, 1) == -1) {
        perror("Error listening");
        return 1;
    }
    printf("Server listening…\n");

    // Loop to control the conecctions
    while(!EXIT_SIGNAL) {
        DEBUG_PRINTF("Number threads: %d", NUM_THREADS);

        // Accept conecction
        socklen_t len = sizeof(client);
        int connfd = accept(socketfd, (struct sockaddr*)&client, &len);
        
        // Check if the maximum number of connections has been reached
        if (NUM_THREADS < CONECCTIONS_LIMIT) {
            // Allocate memory for connfd to avoid overwriting issues
            int *connfd_ptr = malloc(sizeof(int));
            *connfd_ptr = connfd;

            pthread_t thread;
            pthread_create(&thread, NULL, &thread_client, connfd_ptr);
            pthread_detach(thread);

        } else {
            printf("Server capacity full, connection not accepted\n");
            close(connfd);
        }
    }

    close(socketfd);
    return 0;
}

/*Preguntar:
- Problema si accept bloquea hasta siguiente iteracion
*/