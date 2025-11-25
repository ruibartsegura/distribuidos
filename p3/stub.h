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

void reciver(int socket_fd);

void writer(unsigned int id, enum operations mode, int socket_fd);

void reader(unsigned int id, enum operations mode, int socket_fd);

void set_prio(enum operations mode);
#endif // STUB_H
