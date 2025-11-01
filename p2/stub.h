// operaciones.h
#ifndef STUB_H
#define STUB_H

enum operations {
    READY_TO_SHUTDOWN = 0,
    SHUTDOWN_NOW,
    SHUTDOWN_ACK
};

struct message {
    char origin[20];
    enum operations action;
    unsigned int clock_lamport;
};

/* Py start listening Px */
int listen_to(const char px_name[20]);

/* Py establish connection with Px */
int establish_connection(const char px_name[20]);

/* Initialize the server */
int init_coordinator(const char* ip, const int port, const char init_id[20]); // Server

/* Initialize the client */
int join_network(const char* ip, const int port, const char proc_id[20],const char hoster_id[20]); // Client -> server

/* Send a message (op) to another proccess(to_who) */
int communicate(const char to_who[20], enum operations op);

/* Function to get the actual value of the lamport clock */
unsigned int get_clock_lamport();

/* Once the shutdown is confirm close sockets, threads, ... */
void close_everything(); // Close threads & sockets


#endif // STUB_H
