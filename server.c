#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "commun.h"

#define CON_ERR 0
#define SOCKET_ERR -1
#define BUFSZ 1024
#define DISTANCE_SUBTRACT 400
#define RADIUS_EARTH_KM 6371.0
#define SLEEP_TIME 2000000


typedef struct
{
    double latitude;
    double longitude;
} Coordinate;


double track_distance_left(int distance) {
    double distance_left = distance - DISTANCE_SUBTRACT;
    return distance_left;
     
}

double calculate_distance(Coordinate location_driver, Coordinate location_user){
    double dLat = (location_driver.latitude - location_user.latitude) * M_PI / 180.0;
    double dLon = (location_driver.longitude - location_user.longitude) * M_PI / 180.0;
    
    double lat1 = (location_driver.latitude) * M_PI / 180.0;
    double lat2 = (location_user.latitude) * M_PI / 180.0;

    double a = pow(sin(dLat / 2), 2) + pow(sin(dLon / 2), 2) * cos(lat1) * cos(lat2);
    double c = 2 * asin(sqrt(a));

    return RADIUS_EARTH_KM * c;
}

int main(int argc, char **argv)
{
    Coordinate coordServ = {-19.9227,-43.9451};
    int accepted = 0;

    int s;
    struct sockaddr_storage storage;
    printf("Aguardando solicitação.\n");
    // Inicialização da estrutura de endereço de socket do servidor
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage))
    {
        logexit("error argparse");
        exit(EXIT_FAILURE);
    }
    // Criação do socket
    s = socket(storage.ss_family, SOCK_STREAM, 0);

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

    // Aguardando conexões
    if (0 != listen(s, 1))
    {
        logexit("listen");
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        size_t count;
        Coordinate message;
        double remaining_distance;
        struct sockaddr_storage cstorage;
        struct sockaddr *client_sockaddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        // Aceitando conexão de um cliente
        int client_sock = accept(s, client_sockaddr, &caddrlen);
        if (client_sock == -1)
        {
            logexit("conn error");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("Corrida disponivel\n");
            printf("0 - Recusar\n");
            printf("1 - Aceitar\n");
            scanf("%d", &accepted);

            // Recebendo dados do cliente
            count = recv(client_sock, &message, sizeof(Coordinate), 0);
            
            // Calculando a distância restante
            remaining_distance = calculate_distance(coordServ, message);
        }
        while (1)
        {
            if (count == -1)
            {
                break;
            }
            else if (count == 0)
            {
                printf("client disconnected\n");
                close(client_sock);
            }

            if (accepted != 1){
                remaining_distance = -404.00;
                // Enviando resposta para o cliente
                send(client_sock, &remaining_distance, sizeof(double), 0);
                close(client_sock);
                printf("Aguardando solicitação.\n");
                break;

            }else if(remaining_distance <= 0){
                printf("O motorista chegou!\n");
                // Enviando resposta para o cliente
                send(client_sock, &remaining_distance, sizeof(double), 0);
                close(client_sock);
                printf("Aguardando solicitação.\n");
                break;
            }
            else{
                // Enviando resposta para o cliente
                send(client_sock, &remaining_distance, sizeof(double), 0); 
                remaining_distance = track_distance_left(remaining_distance);
                usleep(SLEEP_TIME);
            }
        }
    }

    exit(EXIT_SUCCESS);
}