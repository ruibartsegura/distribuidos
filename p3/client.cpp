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

enum operations {
    WRITE = 0,
    READ
};
struct request {
    enum operations action;
    unsigned int id;
};
struct response {
    enum operations action;
    unsigned int counter;
    long latency_time;
};

void* thread_client(void *arg) {
    // Create socket
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("Error creating socket");
        return NULL;
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
        return NULL;
    }

    // Connect socket
    if (connect(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("Error connecting");
        close(socketfd);
        return NULL;
    }
    printf("connected to the server...\n");
    return NULL;
}

int main (int argc, char* argv[]) {
    // Adjust the argument count and pointer to skip the program name
    argc -= 1;
    argv += 1;

    if (argc != 8) {
        errno = EINVAL; // Invalid argument
        perror("Number of arguments incorrect");
        return EXIT_FAILURE;
    }

    // Match id, ip, mode and threads
    char* ip = NULL;
    int port = -1;
    enum operations mode = NULL;
    int threads = -1;

    if (strcmp(argv[0], "--ip") == 0) {
        ip = argv[1];
    } else {
        errno = EINVAL; // Invalid argument
        perror("Wrong usage: --ip IP --port PORT --mode writer/reader --threads N");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[2], "--port") == 0) {
        port = atoi(argv[3]);
    } else {
        errno = EINVAL; // Invalid argument
        perror("Wrong usage: --ip IP --port PORT --mode writer/reader --threads N");
        return EXIT_FAILURE;
    }

    // Get mode(enum WRITE, READ)
    if (strcmp(argv[4], "--mode") == 0) {
        if (strcmp(argv[5], "writer") == 0)
            mode = WRITE;
        if (strcmp(argv[5], "reader") == 0)
            mode = READ;
    } else {
        errno = EINVAL; // Invalid argument
        perror("Wrong usage: --ip IP --port PORT --mode writer/reader --threads N");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[6], "--threads") == 0) {
        threads = atoi(argv[7]);
    } else {
        errno = EINVAL; // Invalid argument
        perror("Wrong usage: --ip IP --port PORT --mode writer/reader --threads N");
        return EXIT_FAILURE;
    }


    for (int t = 0; t < threads, t++) {


        pthread_t thread;
        int status = pthread_create(&thread, NULL, &thread_reciver, socket_fd_ptr);

        // Check the correct creatin of the thread
        if (status != 0) {
            perror("pthread_create failed");
            return EXIT_FAILURE;
        }

        thread_fd[thread_count] = thread; // Add the thread fd to the list
        thread_count++;
        }

    return 0;
}