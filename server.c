#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "commun.h"

#define CON_ERR 0
#define SOCKET_ERR -1+6

#define BUFSZ 2048
#define DISTANCE_SUBTRACT 400
#define RADIUS_EARTH_KM 6371.0
#define COUNT_INTERVAL 4
#define UPDATE_INTERVAL 3

#define MAX_MOVIES 4
#define MAX_STRINGS 7
#define MAX_LENGTH 100


// Handlers for the sending and receiving threads.
int recv_handler(void *info);
int send_handler(void *info);
int count_handler(void *info);

typedef struct client_list *List;
typedef struct args *Args;

typedef struct client_node *Node;

struct args{
    List list;
    int fd;
} ;

Args new_args(List list, int fd);


struct client_list{
    Node head, tail;
    cnd_t t_lock;
    mtx_t mutex;
};

struct client_info {
    char host[INET_ADDRSTRLEN];
    int port;
    int movie;
    int line;
};

struct client_node {
    struct client_info client;
    Node next, prev;
};


void manage_movie_list(char movie_list[MAX_MOVIES][MAX_STRINGS][MAX_LENGTH]) {
    // Initialize and populate the movie list
    strcpy(movie_list[1][5], "Um anel para a todos governar");
    strcpy(movie_list[1][4], "Na terra de Mordor onde as sombras se deitam");
    strcpy(movie_list[1][3], "Não é o que temos, mas o que fazemos com o que temos");
    strcpy(movie_list[1][2], "Não há mal que sempre dure");
    strcpy(movie_list[1][1], "O mundo está mudando, senhor Frodo");
    strcpy(movie_list[1][0], "");

    strcpy(movie_list[2][5], "Vou fazer uma oferta que ele não pode recusar");
    strcpy(movie_list[2][4], "Mantenha seus amigos por perto e seus inimigos mais perto ainda");
    strcpy(movie_list[2][3], "É melhor ser temido que amado");
    strcpy(movie_list[2][2], "A vingança é um prato que se come frio");
    strcpy(movie_list[2][1], "Nunca deixe que ninguém saiba o que você está pensando");
    strcpy(movie_list[2][0], "");

    strcpy(movie_list[3][5], "Primeira regra do Clube da Luta: você não fala sobre o Clube da Luta");
    strcpy(movie_list[3][4], "Segunda regra do Clube da Luta: você não fala sobre o Clube da Luta");
    strcpy(movie_list[3][3], "O que você possui acabará possuindo você");
    strcpy(movie_list[3][2], "É apenas depois de perder tudo que somos livres para fazer qualquer coisa");
    strcpy(movie_list[3][1], "Escolha suas lutas com sabedoria");
    strcpy(movie_list[3][0], "");
}

int main(int argc, char **argv)
{
    int s, c;
    struct sockaddr_storage storage;
    printf("Aguardando solicitação.\n");
    // Inicialização da estrutura de endereço de socket do servidor
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage))
    {
        logexit("error argparse");
        exit(EXIT_FAILURE);
    }
    // Criação do socket
    s = socket(storage.ss_family, SOCK_DGRAM, 0);
    c = socket(storage.ss_family, SOCK_DGRAM, 0);

    if (s == -1)
    {
        logexit("socket");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    // Habilitação da opção SO_REUSEADDR no socket
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0)
    {
        logexit("error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr *address = (struct sockaddr *)(&storage);

    // Associação do socket a um endereço
    if (0 != bind(s, address, sizeof(storage)))
    {
        logexit("bind");
        exit(EXIT_FAILURE);
    }

    List list = calloc(1, sizeof(*list));

    cnd_init(&list->t_lock);
    mtx_init(&list->mutex, mtx_plain);

    Args server_info = new_args(list, s);
    Args client_info = new_args(list, c);

    // Create the threads.
    thrd_t recv_thread;
    thrd_t send_thread;
    thrd_t count_thread;

    thrd_create(&recv_thread, recv_handler, (void *) server_info);
    thrd_create(&send_thread, send_handler, (void *) client_info);
    thrd_create(&count_thread, count_handler, (void *) client_info);

    while (1)
    {
        sleep(100);
    }

    // Close the sockets
    close(s);
    close(c);

    // Clean up the threads.
    int retval;
    thrd_join(recv_thread, &retval);
    thrd_join(send_thread, &retval);
    thrd_join(count_thread, &retval);

     for (Node curr = list->head; curr != NULL;) {
        Node tmp = curr;
        curr = curr->next;
        free(tmp);
    }

    exit(EXIT_SUCCESS);
}

int recv_handler(void *args_) {
    Args args = (Args) args_;
    List list = args->list;
    int server_fd = args->fd;

    char movie_list[MAX_MOVIES][MAX_STRINGS][MAX_LENGTH];
    manage_movie_list(movie_list);

    // Array that we'll use to store the data we're sending / receiving.
    char buf[BUFSZ + 1] = {0};

    // Create the sockaddr that the server will use to receive data from
    // the client (and to then send data back).
    struct sockaddr_in sockaddr_from;

    // We need to create a variable to store the length of the sockaddr
    // we're using here, which the recvfrom function can update if it
    // stores a different amount of information in the sockaddr.
    socklen_t from_len = sizeof(sockaddr_from);

    while (1) {

        int numbytes = recvfrom(server_fd, buf, BUFSZ, 0, (struct sockaddr *) &sockaddr_from, &from_len);
        
        buf[numbytes] = '\0';
        buf[strcspn(buf, "\n")] = '\0';

        // Create a struct with the information about the client
        // (host, port) -- similar to "clientAddress" in the Python.
        struct client_info client;
        client.port = ntohs(sockaddr_from.sin_port);
        inet_ntop(sockaddr_from.sin_family, &(sockaddr_from.sin_addr), client.host, INET_ADDRSTRLEN);

        mtx_lock(&(list->mutex));

        int loop_control = atoi(buf);

        if (loop_control != 0) {
            Node node = calloc(1, sizeof(*node));
            node->client = client;
            node->client.movie = loop_control;
            node->client.line = 5;
            if (list->tail == NULL) {
                list->head = list->tail = node;
            } else {
                list->tail->next = node;
                node->prev = list->tail;
                list->tail = node;
            }
            strcpy(buf, movie_list[node->client.movie][node->client.line]);

        } else {
            strcpy(buf, "Unknown command, send Subscribe or Unsubscribe only");
        }

        // The `send_wrapper` function wraps the call to sendto, to
        // avoid code duplication.

        sendto(server_fd, buf, strlen(buf), 0, (struct sockaddr *) &sockaddr_from, *&from_len);

        // Now that we're finished, we want to notify the waiting thread
        // and release the lock.

        // Wake up one waiting thread.
        // This is the equivalent of `t_lock.notify()` in the Python code:

        cnd_signal(&(list->t_lock));

        // And unlock the mutex now that we're done.
        // This is the equivalent of the end of the `with t_lock:` block
        mtx_unlock(&(list->mutex));
    }

    return EXIT_SUCCESS;
}

int send_handler(void *args_) {
    Args args = (Args) args_;
    List list = args->list;
    int client_fd = args->fd;

    char movie_list[MAX_MOVIES][MAX_STRINGS][MAX_LENGTH];
    manage_movie_list(movie_list);

    // Array that we'll use to store the data we're sending / receiving.
    char buf[BUFSZ + 1] = {0};

    while (1) {

        // Get the lock.
        mtx_lock(&(list->mutex));

        // For each client:
        for (Node curr = list->head; curr != NULL; curr = curr->next) {

            struct sockaddr_storage sockaddr_to = {0};
            socklen_t to_len = sizeof(sockaddr_to);
            char portstr[6]; // Maximum length of a port number as a string (65535) plus null terminator
            snprintf(portstr, sizeof(portstr), "%d", curr->client.port);
            addrparse(curr->client.host, portstr, &sockaddr_to);

            if (curr->client.line == 0){

                if (list->head == curr) {
                    list->head = curr->next;
                }

                if (list->tail == curr) {
                    list->tail = curr->prev;
                }

                if (curr->next) curr->next = curr->next->next;
                if (curr->prev) curr->prev = curr->prev->prev;
                sendto(client_fd, "Ended", strlen(buf), 0, (struct sockaddr *) &sockaddr_to, to_len);
                free(curr);
            }else{
                strcpy(buf, movie_list[curr->client.movie][curr->client.line]);
                curr->client.line--;
                sendto(client_fd, buf, strlen(buf), 0, (struct sockaddr *) &sockaddr_to, to_len);
            }
        }
        cnd_signal(&(list->t_lock));

        mtx_unlock(&(list->mutex));

        // sleep for UPDATE_INTERVAL
        sleep(UPDATE_INTERVAL);

    }
    return EXIT_SUCCESS;
}

int count_handler(void *args_){
    Args args = (Args) args_;
    List list = args->list;
    while (1) {
        mtx_lock(&(list->mutex));
        int conn_number = 0;
        for (Node curr = list->head; curr != NULL; curr = curr->next) {
            conn_number++;
        }
        cnd_signal(&(list->t_lock));

        mtx_unlock(&(list->mutex));

        printf("Clientes: %d\n", conn_number);
        sleep(COUNT_INTERVAL);
    }
    return EXIT_SUCCESS;
}

Args new_args(List list, int fd) {
    Args args = calloc(1, sizeof(*args));
    args->list = list;
    args->fd = fd;
    return args;
}