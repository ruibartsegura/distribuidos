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

int init_coordinator(const int port); // Server

int join_network(const char* ip, const int port); // Client -> server

void ready_shutdown(const char origin[20], const int lamport_count);

void shutdown_confirm(const char origin[20], const int lamport_count);

void shutdown_done(const char origin[20], const int lamport_count);

int get_clock_lamport();


#endif // STUB_H
