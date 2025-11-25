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
char p3[20] = "P3";

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

    // Initialize the coordinator of communication
    if (init_coordinator(ip, port, p2) == EXIT_FAILURE) {
        perror("Initialization failed");
        return EXIT_FAILURE;
    }
    
    // Establish_connection P2 -> P1
    if (establish_connection(p1) == EXIT_FAILURE) {
        perror("Connection failed");
        return EXIT_FAILURE;
    }
    
    // Listen
    if (listen_to(p1) == EXIT_FAILURE) {
        perror("Listening failed");
        return EXIT_FAILURE;
    }

    while (get_clock_lamport() != 2) {
        usleep(10000);
        continue;
    }
    
    // Establish_connection P2 -> P3
    if (establish_connection(p3) == EXIT_FAILURE) {
        perror("Connection failed");
        return EXIT_FAILURE;
    }

    // Listen
    if (listen_to(p3) == EXIT_FAILURE) {
        perror("Listening failed");
        return EXIT_FAILURE;
    }

    while (get_clock_lamport() != 3) {
        usleep(10000);
        continue;
    }

    // Communicate the shutdown ack
    if (communicate(p1, SHUTDOWN_NOW) == EXIT_FAILURE) {
        perror("Communication failed");
        return EXIT_FAILURE;
    }

    // Listen
    if (listen_to(p1) == EXIT_FAILURE) {
        perror("Listening failed");
        return EXIT_FAILURE;
    }

    while (get_clock_lamport() != 7) {
        usleep(10000);
        continue;
    }

    // Communicate the shutdown ack
    if (communicate(p3, SHUTDOWN_NOW) == EXIT_FAILURE) {
        perror("Communication failed");
        return EXIT_FAILURE;
    }

    // Listen
    if (listen_to(p3) == EXIT_FAILURE) {
        perror("Listening failed");
        return EXIT_FAILURE;
    }

    while (get_clock_lamport() != 11) {
        usleep(10000);
        continue;
    }

    // Close
    close_everything();
    return EXIT_SUCCESS;
}