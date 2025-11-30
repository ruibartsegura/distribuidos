// operaciones.h
#ifndef STUB_H
#define STUB_H

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

// Server
void finish();

void check_counter();
void set_prio(enum operations mode);

void accept_conections(int server_socket);

// Client
void writer(unsigned int id, enum operations mode, int socket_fd);

void reader(unsigned int id, enum operations mode, int socket_fd);

#endif // STUB_H
