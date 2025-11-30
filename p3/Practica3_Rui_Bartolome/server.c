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

bool EXIT_SIGN = false; // Bool to exit with control

void handle_sigint(int sig) {
    finish();
    EXIT_SIGN = true;
}

int main (int argc, char* argv[]) {
    signal(SIGINT, handle_sigint); // Handle CTRL C
    setbuf(stdout, NULL); // Avoid buffering

    // Get params
    static struct option long_options[] = {
        {"port",    required_argument, 0,  1},
        {"priority", required_argument, 0,  2},
        {0, 0, 0, 0}
    };

    int opt, option_index = 0;
    int port = -1;
    bool prio_seted = false;

    while ((opt = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        switch (opt) {
            case 1: // port
                port = atoi(optarg);
                break;

            case 2: // priority
                if (strcmp(optarg, "writer") == 0)
                    set_prio(WRITE);
                else if (strcmp(optarg, "reader") == 0)
                    set_prio(READ);
                prio_seted = true;
                break;
        }
    }

    if (port == -1 || prio_seted == false) {
        fprintf(stderr,
                "Use: --port <puerto> --prio <reader|writer>\n");
        exit(1);
    }

    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    // Define and configure the server address structure
    struct sockaddr_in server_addr;
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

    // Listen
    if (listen(server_socket, 10000) == -1) {
        perror("Error listening");
        return EXIT_FAILURE;
    }

    // Check if there is a initial value for the counter
    check_counter();
    accept_conections(server_socket);
   
    return EXIT_SUCCESS;
}