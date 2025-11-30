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

struct thread_args {
    char* ip;
    int port;
    enum operations mode;
    int id;
};


void handle_sigint(int sig) {
    finish();
}

void* thread_client(void *arg) {
    // Create socket
    struct thread_args thread_arg = *((struct thread_args *)arg);
    free(arg);

    // Create socket
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("Error creating socket");
        return NULL;
    }

    // Define and configure the server address structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(thread_arg.port);

    // Convert the IP address from text to binary form and validate it
    if (inet_pton(AF_INET, thread_arg.ip, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address: %s\n", thread_arg.ip);
        close(socketfd);
        return NULL;
    }

    // Connect socket
    if (connect(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("Error connecting");
        close(socketfd);
        return NULL;
    }

    // Sendo msg
    if (thread_arg.mode == WRITE) {
        writer(thread_arg.id, thread_arg.mode, socketfd);
        
    } else if (thread_arg.mode == READ) {
        reader(thread_arg.id, thread_arg.mode, socketfd);

    } else {
        perror("No mode selected");
        close(socketfd);
        return NULL;
    }
    return NULL;
}

int main (int argc, char* argv[]) {
    signal(SIGINT, handle_sigint); // Handle CTRL C
    setbuf(stdout, NULL); // Avoid buffering

    // Match id, ip, mode and threads

    static struct option long_options[] = {
        {"ip",      required_argument, 0,  1},
        {"port",    required_argument, 0,  2},
        {"mode",    required_argument, 0,  3},
        {"threads", required_argument, 0,  4},
        {0, 0, 0, 0}
    };

    int opt, option_index = 0;

    // initialize program values
    struct thread_args thread_arg;
    thread_arg.ip = NULL;
    thread_arg.port = -1;
    thread_arg.mode = -1;
    int threads = -1;


    while ((opt = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        switch (opt) {
            case 1: // ip
                thread_arg.ip = optarg;
                break;

            case 2: // port
                thread_arg.port = atoi(optarg);
                break;

            case 3: // mode
                if (strcmp(optarg, "writer") == 0)
                    thread_arg.mode = WRITE;
                else if (strcmp(optarg, "reader") == 0)
                    thread_arg.mode = READ;
                else {
                    fprintf(stderr, "modo inválido\n");
                    exit(1);
                }
                break;

            case 4: // threads
                threads = atoi(optarg);
                break;

            default:
                printf("Unknown option\n");
        }
    }

    if (thread_arg.ip == NULL ||
        thread_arg.port == -1 ||
        thread_arg.mode == -1 ||
        threads <= 0) {

        fprintf(stderr,
                "Use: --ip <ip> --port <puerto> --mode <reader|writer> --threads <n>\n");
        exit(1);
    }

    pthread_t thread_fd[threads]; // Will save the fd of the trhead to close everything at the end

    for (int id = 0; id < threads; id++) {
        struct thread_args* args = malloc(sizeof(struct thread_args));
        *args = thread_arg;      // copy config (ip, port, mode)
        args->id = id;           // assign unique id

        pthread_t thread;
        int status = pthread_create(&thread, NULL, thread_client, args);

        if (status != 0) {
            perror("pthread_create failed");
            return EXIT_FAILURE;
        }
        thread_fd[id] = thread;
    }

    // Esperar a que todos los threads terminen
    for (int id = 0; id < threads; id++) {
        pthread_join(thread_fd[id], NULL);
    }

    return 0;
}
