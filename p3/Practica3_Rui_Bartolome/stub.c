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
#include <time.h>
#include "stub.h"

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

#define CONECCTIONS_LIMIT 600

// Random between
#define MIN 75
#define MAX 150

char *path = "server_output.txt";

struct socket_pos {
    int socket_fd;
    int pos;
};

struct thread_args {
    struct request req;
    struct socket_pos sp;
};


// Thread and sockets fd
pthread_t thread_fd[CONECCTIONS_LIMIT];
int server_sockets_fd[CONECCTIONS_LIMIT];

// Mutex and conditions
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_writer = PTHREAD_COND_INITIALIZER; // Cond for Writers
pthread_cond_t cond_reader = PTHREAD_COND_INITIALIZER; // Cond for Readers

int waiting_writer = 0;
int waiting_reader = 0;

int active_writers = 0;
int active_readers = 0;

// To know which priority is active
enum operations prio;

int free_spots[CONECCTIONS_LIMIT];
bool spot_available = true;
pthread_mutex_t spots_mutex;   // mutex para protegerlo

// Counter of writers
int counter = 0;

// Exit signal
bool EXIT_SIGNAL = false; // Bool to exit with control

// When the signal is recived in client or server activate this
void finish() {
    EXIT_SIGNAL = true;
}

// Function to generate the random wait of the server (75ms-150ms)
float random_generator(unsigned int *seed) {
    float num;
    num = MIN + ((float)rand_r(seed) / RAND_MAX) * (MAX - MIN);
    return num;
}

// Make the random sleep
void rand_sleep() {
    // Sleep a random time between 75 - 150 ms
    // Get a seed for random, thread safe
    unsigned int seed = time(NULL) ^ pthread_self();
    float num = random_generator(&seed); // Random value

    // Sleep the random period
    usleep((useconds_t)(num * 1000)); // *1000 to make it ms
}

// Get the name of the action from it enum
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

// Control the priorities
void* handler(void *arg) {
    while (!EXIT_SIGNAL) {
        if (prio == WRITE) {
                // Handle first writers one by one, if there is no writer the let pass 
                // all the readers
                if (waiting_writer > 0) {
                    pthread_cond_signal(&cond_writer);
                    usleep(5000);
                } else {
                    pthread_cond_broadcast(&cond_reader);
                    usleep(5000);
                }
            }

            if (prio == READ) {
                if (waiting_reader > 0 && active_writers == 0) {
                    pthread_cond_broadcast(&cond_reader);
                    usleep(5000);
                }
                else if (waiting_writer > 0 && waiting_reader == 0) {
                    pthread_cond_signal(&cond_writer);
                    usleep(5000);
                }
            }
    }
    return NULL;
}

// Set the prio and activate handler
void set_prio (enum operations mode) {
    prio = mode;

    // Activate handler
    pthread_t thread;
    int status = pthread_create(&thread, NULL, &handler, NULL);

    // Check the correct creation of the thread
    if (status != 0) {
        perror("pthread_create failed");
        return;
    }
}

// Check if the counter is initializated
void check_counter() {
    // Read the file
    FILE *f = fopen(path, "r");
    if (f) {
        char buffer[64];
        if (fgets(buffer, sizeof(buffer), f)) {
            counter = atoi(buffer);
        }
        fclose(f);
    }
}

// Writer
void writer(unsigned int id, enum operations mode, int socket_fd) {
    struct request req;
    memset(&req, 0, sizeof(req));

    req.action = mode;
    req.id = id;

    // SEND
    if (send(socket_fd, &req, sizeof(req), 0) == -1) {
        perror("Error sending msg");
        close(socket_fd);
        return;
    }

    // RECV
    struct response msg_2_rcv;
    if (recv(socket_fd, &msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
        perror("Error reciving msg");
        close(socket_fd);
        return ;
    }

    printf("[Cliente #%d] %s, contador=%d, tiempo=%ld ns.\n",
            id, get_action_name(msg_2_rcv.action), msg_2_rcv.counter,
            msg_2_rcv.latency_time);

    close(socket_fd);
    return;
}

// Reader
void reader(unsigned int id, enum operations mode, int socket_fd) {
    struct request req;
    memset(&req, 0, sizeof(req)); // Clean posible mem

    // Match id and mode
    req.action = mode;
    req.id = id;

    // SEND
    if (send(socket_fd, &req, sizeof(req), 0) == -1) {
        perror("Error sending msg");
        close(socket_fd);
        return;
    }

    // RECV
    struct response msg_2_rcv;
    if (recv(socket_fd, &msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
        perror("Error reciving msg");
        close(socket_fd);
        return ;
    }
    
    printf("[Cliente #%d] %s, contador=%d, tiempo=%ld ns.\n",
        id, get_action_name(msg_2_rcv.action), msg_2_rcv.counter,
        msg_2_rcv.latency_time);

    close(socket_fd);
    return;
}

void* thread_writer(void *arg) {
    struct thread_args thread_arg = *((struct thread_args *)arg);
    free(arg);

    // Struct to get latency
    struct timespec start, end, t;

    // To store latency
    long latency;

    clock_gettime(CLOCK_REALTIME, &start);   // Start of trying to get the critic zone

    // Get mutex
    pthread_mutex_lock(&mutex);
    
    waiting_writer++; // add one writer to the waiting list

    while (active_writers > 0 || active_readers > 0) {
        pthread_cond_wait(&cond_writer, &mutex);
    }

    waiting_writer--;
    active_writers++;

    clock_gettime(CLOCK_REALTIME, &end); // Has eter in critic zone

    // Get latency
    latency =
        (end.tv_sec - start.tv_sec) * 1000000000L +
        (end.tv_nsec - start.tv_nsec);

    counter++;

    FILE *f = fopen("server_output.txt", "w");

    if (f == NULL) {
        printf("No existe el archivo o no se puede abrir.\n");
        return NULL;
    }

    fprintf(f, "%d\n", counter);

    fclose(f);

    pthread_mutex_unlock(&mutex);


    // Do the sleep
    rand_sleep();

    // Msg to send
    struct response msg_2_send;
    msg_2_send.action = thread_arg.req.action;
    msg_2_send.counter = counter;
    msg_2_send.latency_time = latency;

    // SEND
    if (send(thread_arg.sp.socket_fd, &msg_2_send, sizeof(msg_2_send), 0) == -1) {
        perror("Error sending msg");
        close(thread_arg.sp.socket_fd);

        pthread_mutex_lock(&spots_mutex);
        server_sockets_fd[thread_arg.sp.pos] = -1;
        free_spots[thread_arg.sp.pos] = -1;
        pthread_mutex_unlock(&spots_mutex);

        return NULL;
    }

    // get actual time
    clock_gettime(CLOCK_REALTIME, &t);
    printf("[%ld][Escritor %d] modifica contador con valor %d\n",
            t.tv_sec, thread_arg.req.id, counter);

    pthread_mutex_lock(&mutex);
    active_writers--;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void* thread_reader(void *arg) {
    struct thread_args thread_arg = *((struct thread_args *)arg);
    free(arg);

    // Value of counter
    int count = 0;

    // Struct to get latency
    struct timespec start, end, t;

    // To store latency
    long latency;

    clock_gettime(CLOCK_REALTIME, &start);   // Start of trying to get the critic zone
    
    // Get mutex
    pthread_mutex_lock(&mutex);
    
    DEBUG_PRINTF("Entro mutex\n");
    waiting_reader++; // add one reader to the waiting list

    while (active_writers > 0) {
        pthread_cond_wait(&cond_reader, &mutex);
    }

    clock_gettime(CLOCK_REALTIME, &end); // Has eter in critic zone
    
    waiting_reader--;
    active_readers++;

    DEBUG_PRINTF("active\n");

    // Get latency
    latency =
    (end.tv_sec - start.tv_sec) * 1000000000L +
    (end.tv_nsec - start.tv_nsec);
    
    pthread_mutex_unlock(&mutex);
        
    // Do the sleep
    rand_sleep();
    
    // Read the file
    FILE *f = fopen(path, "r");
    

    if (f) {
        char buffer[64];
        if (fgets(buffer, sizeof(buffer), f)) {
            count = atoi(buffer);
        }
        fclose(f);
    }
    
    // Msg to send
    struct response msg_2_send;
    msg_2_send.action = thread_arg.req.action;
    msg_2_send.counter = count;
    msg_2_send.latency_time = latency;
    
    // SEND
    if (send(thread_arg.sp.socket_fd, &msg_2_send, sizeof(msg_2_send), 0) == -1) {
        perror("Error sending msg");
        close(thread_arg.sp.socket_fd);

        pthread_mutex_lock(&spots_mutex);
        server_sockets_fd[thread_arg.sp.pos] = -1;
        free_spots[thread_arg.sp.pos] = -1;
        pthread_mutex_unlock(&spots_mutex);
        
        return NULL;
    }

    // get actual time
    clock_gettime(CLOCK_REALTIME, &t);
    printf("[%ld][LECTOR %d] lee contador con valor %d\n",
        t.tv_sec, thread_arg.req.id, count);

    pthread_mutex_lock(&mutex);
    active_readers--;
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

void* thread_server(void *arg) {
    struct socket_pos sp = *((struct socket_pos *)arg);
    free(arg);

    struct request msg_2_rcv;
    // Recive msg
    if (recv(sp.socket_fd, &msg_2_rcv, sizeof(msg_2_rcv), 0) == -1) {
        perror("Error reciving msg");
        close(sp.socket_fd);

        pthread_mutex_lock(&spots_mutex);
        server_sockets_fd[sp.pos] = -1;
        free_spots[sp.pos] = -1;
        pthread_mutex_unlock(&spots_mutex);

        return NULL;
    }

    struct thread_args* args = malloc(sizeof(struct thread_args));
    args->req = msg_2_rcv;
    args->sp = sp;


    pthread_t thread;
    if (msg_2_rcv.action == WRITE) {
        int status = pthread_create(&thread, NULL, &thread_writer, args);

        // Check the correct creation of the thread
        if (status != 0) {
            perror("pthread_create failed");
            close(sp.socket_fd);

            pthread_mutex_lock(&spots_mutex);
            server_sockets_fd[sp.pos] = -1;
            free_spots[sp.pos] = -1;
            pthread_mutex_unlock(&spots_mutex);

            return NULL;
        }
    } else if (msg_2_rcv.action == READ) {
        int status = pthread_create(&thread, NULL, &thread_reader, args);

        // Check the correct creation of the thread
        if (status != 0) {
            perror("pthread_create failed");
            close(sp.socket_fd);
            
            pthread_mutex_lock(&spots_mutex);
            server_sockets_fd[sp.pos] = -1;
            free_spots[sp.pos] = -1;
            pthread_mutex_unlock(&spots_mutex);

            return NULL;
        }
    }

    pthread_join(thread, NULL);

    // This socket has reach the end so close everything
    close(sp.socket_fd);

    pthread_mutex_lock(&spots_mutex);
    server_sockets_fd[sp.pos] = -1;
    free_spots[sp.pos] = -1;
    pthread_mutex_unlock(&spots_mutex);

    return NULL;
}

void accept_conections(int server_socket) {
    struct sockaddr_in client;

    for (int x = 0; x < CONECCTIONS_LIMIT; x++) {
        free_spots[x] = -1;
    }
    // Loop to control the conecctions
    while (!EXIT_SIGNAL) {
        
        pthread_mutex_lock(&spots_mutex);
        spot_available = false;
        for (int x = 0; x < CONECCTIONS_LIMIT; x++) {
            if (free_spots[x] == -1) {
                spot_available = true;
                break;
            }
        }
        pthread_mutex_unlock(&spots_mutex);


        // Check if the maximum number of connections has been reached
        while (spot_available) {
            // Accept conecction
            socklen_t len = sizeof(client);

            int connfd = accept(server_socket, (struct sockaddr*)&client, &len);
            if (connfd == -1) {
                perror("Error accepting");
                close(server_socket);
                return;
            }
            
            struct socket_pos *sp = malloc(sizeof(struct socket_pos));
            pthread_mutex_lock(&spots_mutex);
            sp->pos = -1;
            for (int x = 0; x < CONECCTIONS_LIMIT; x++) {
                if (free_spots[x] == -1) {
                    sp->pos = x;
                    sp->socket_fd = connfd;
                    break;
                }
            }
            if (sp->pos == -1) {
                perror("No pos finded");
                close(server_socket);
                return;
            }
            pthread_mutex_unlock(&spots_mutex);

            pthread_t thread;
            int status = pthread_create(&thread, NULL, &thread_server, sp);

            // Check the correct creation of the thread
            if (status != 0) {
                perror("pthread_create failed");
                close(server_socket);
                return;
            }

            thread_fd[sp->pos] = thread;
            server_sockets_fd[sp->pos] = connfd;

            if (EXIT_SIGNAL)
                break;
        }

    }
    for (int x = 0; x < CONECCTIONS_LIMIT; x++) {
        if (server_sockets_fd[x] != -1 && pthread_join( thread_fd[x], NULL) != 0) {
            perror("pthread_join failed");
            close(server_socket);
            return;
        }
        close(server_sockets_fd[x]);
    }
    close(server_socket);
}
