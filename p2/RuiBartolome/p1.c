#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "stub.h"

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

/*Global variables*/
char p1[20] = "P1";
char p2[20] = "P2";

int main (int argc, char* argv[]) {
    // Adjust the argument count and pointer to skip the program name
    argc -= 1;
    argv += 1;
    if (argc != 2) {
        errno = EINVAL; // Invalid argument
        perror("Number of arguments incorrect");
        return EXIT_FAILURE;
    }

    // Match id, ip, port with the arguments
    char* ip = argv[0];
    int port = atoi(argv[1]);

    // Connect P1 -> P2
    if (join_network(ip, port, p1, p2) == EXIT_FAILURE) {
        perror("Conexion to the network failed");
        return EXIT_FAILURE;
    }

    // Communicate that its ready to shutdown
    if (communicate(p2, READY_TO_SHUTDOWN) == EXIT_FAILURE) {
        perror("Communication failed");
        return EXIT_FAILURE;
    }

    while (get_clock_lamport() != 1) {
        usleep(10000);
        continue;
    }

    // Listen the response
    if (listen_to(p2) == EXIT_FAILURE) {
        perror("Listening failed");
        return EXIT_FAILURE;
    }

    while (get_clock_lamport() != 5) {
        usleep(10000);
        continue;
    }

    // Communicate the shutdown ack
    if (communicate(p2, SHUTDOWN_ACK) == EXIT_FAILURE) {
        perror("Communication failed");
        return EXIT_FAILURE;
    }

    close_everything();
    return EXIT_SUCCESS;
}