#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "stub.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_writer = PTHREAD_COND_INITIALIZER; // Cond for Writers
pthread_cond_t cond_reader = PTHREAD_COND_INITIALIZER; // Cond for Readers

int waiting_writer = 0;
int waiting_reader = 0;

int actives_writer = 0;
int actives_reader = 0;

enum operations prio;

int counter = 0;

char* get_action_name(int act_id){
    switch (act_id) {
        case WRITE:
            return "WRITE";
        case READ:
            return "READ";
        default: 
            return "UNKNOWN";
    }
}

void handler() {
    while (1) {
        if (strcmp(get_action_name(prio), "WRITE") == 0) {
            // Handle first writers one by one, if there is no writer the let pass 
            // all the readers
            if (waiting_writer > 0) {
                pthread_cond_signal(&cond_writer);
            } else {
                pthread_cond_broadcast(&cond_reader);
            }
        } else {
            // Handle first reader all in one, if there is no reader the let pass 
            // all the writers
            if (waiting_reader > 0) {
                pthread_cond_broadcast(&cond_reader);
            } else {
                pthread_cond_signal(&cond_writer);
            }
        }
    }
}

void* thread_writer(void *arg) {
    // Create socket
    struct request req_arg = *((struct request *)arg);
    free(arg);

    pthread_mutex_lock(&mutex);

    waiting_writer++;

    pthread_cond_wait(&cond_writer, &mutex);

    waiting_writer--;
    actives_writer++;

    pthread_mutex_unlock(&mutex);


    // Ejecutar writer

    return NULL;
}

void* thread_reader(void *arg) {
    // Create socket
    struct request req_arg = *((struct request *)arg);
    free(arg);

    pthread_mutex_lock(&mutex);

    waiting_reader++;

    if (actives_reader == 0)
        pthread_cond_wait(&cond_reader, &mutex);

    waiting_reader--;
    actives_reader++;

    pthread_mutex_unlock(&mutex);

    return NULL;
}


void set_prio (enum operations mode) {
    prio = mode;
}

void writer(unsigned int id, enum operations mode, int socket_fd) {
    struct request req;
    req.action = mode;
    req.id = id;

    memset(&req, 0, sizeof(req));

    // SEND
    if (send(socket_fd, &req, sizeof(req), 0) == -1) {
        perror("Error sending msg");
        close(socket_fd);
        return;
    }

    memset(&req, 0, sizeof(req));

    // RECV
    struct response msg_2_rcv;
    if (recv(socket_fd, &msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
        perror("Error reciving msg");
        close(socket_fd);
        return ;
    }

    printf("[Cliente #%d] %s, contador=%d, tiempo=%ld ns.",
            id, get_action_name(msg_2_rcv.action), msg_2_rcv.counter,
            msg_2_rcv.latency_time);

    return;
}


void reader(unsigned int id, enum operations mode, int socket_fd) {
    struct request req;
    req.action = mode;
    req.id = id;

    memset(&req, 0, sizeof(req));

    // SEND
    if (send(socket_fd, &req, sizeof(req), 0) == -1) {
        perror("Error sending msg");
        close(socket_fd);
        return;
    }

    memset(&req, 0, sizeof(req));

    // RECV
    struct response msg_2_rcv;
    if (recv(socket_fd, &msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
        perror("Error reciving msg");
        close(socket_fd);
        return ;
    }

    printf("[Cliente #%d] %s, contador=%d, tiempo=%ld ns.",
            id, get_action_name(msg_2_rcv.action), msg_2_rcv.counter,
            msg_2_rcv.latency_time);
    return;
}

void reciver (int socket_fd) {
    struct request msg_2_rcv;
    // Recive msg
    if (recv(socket_fd, &msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
        perror("Error reciving msg");
        close(socket_fd);
        return;
    }

    if (msg_2_rcv.action == WRITE) {
        pthread_t thread;
        int status = pthread_create(&thread, NULL, &thread_writer, &msg_2_rcv);

        // Check the correct creation of the thread
        if (status != 0) {
            perror("pthread_create failed");
            close(socket_fd);
            return;
        }
    } else if (msg_2_rcv.action == READ) {
        pthread_t thread;
        int status = pthread_create(&thread, NULL, &thread_reader, &msg_2_rcv);

        // Check the correct creation of the thread
        if (status != 0) {
            perror("pthread_create failed");
            close(socket_fd);
            return;
        }
    }
}

/*
reciver crea un thread de writer o reader:
    Writer -> +1 contador (CRITICO)
           -> Escribir en server_output.txt (CRITCO)
           -> printf T. Actual Escritor N y contador (CRITICO)
    Reader -> Print Contador (CRITICO)

*/