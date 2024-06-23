#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "commun.h"

#define CON_ERR 0
#define SOCKET_ERR -1
#define BUFSZ 1024

typedef struct {
    double latitude;
    double longitude;
} Coordinate;


int main(int argc, char **argv)
{
    Coordinate coordCli = {-19.8307, -43.9372};
    Coordinate message;

    int loop_control = 1;
    while (1)
    {
        printf("0 - Sair\n");
        printf("1 - Senhor dos aneis\n");
        printf("2 - O poderose Chefao\n");
        printf("3 - Clube da luta\n");
        scanf("%d", &loop_control);

        if (loop_control == 0){
            break;
        }

        int s;

        char addressStr[BUFSZ];

        struct sockaddr_storage storage;
        // Converte o endereço IP e a porta para uma estrutura de endereço de socket
        if (0 != addrparse(argv[2], argv[3], &storage))
        {
            logexit("error argparse");
            exit(EXIT_FAILURE);
        }
        // Cria um novo socket
        s = socket(storage.ss_family, SOCK_STREAM, 0);

        if (s == SOCKET_ERR)
        {
            logexit("socket");
            exit(EXIT_FAILURE);
        }

        struct sockaddr *address = (struct sockaddr *)(&storage);

        // Estabelece uma conexão com o servidor
        if (CON_ERR != connect(s, address, sizeof(storage)))
        {
            logexit("connect");
            exit(EXIT_FAILURE);
        }

        // Converte a estrutura de endereço de socket para uma string
        addrtostr(address, addressStr, BUFSZ);

        // Converte os valores de latitude e longitude para o formato de rede
        message.latitude = htons(coordCli.latitude);
        message.longitude = htons(coordCli.longitude);

        // Envia os dados de coordenadas para o servidor
        ssize_t bytes_sent = send(s, &message, sizeof(Coordinate), 0);
        if (bytes_sent == -1){
            logexit("Error sending the message");
            exit(1);
        }

        double response;
        // Recebe a resposta do servidor
        ssize_t bytes_received = recv(s, &response, sizeof(double), 0);
        if (bytes_received == -1)
        {
            logexit("Error receiving the response");
            exit(1);
        }

        // Verifica se a resposta indica que não há motorista disponível
        if (response == -404.00){
            printf("Não foi encontrado um motorista.\n");
            close(s);
            continue;
        }
        // Exibe a distância do motorista até o cliente
        while(response > 0){
            printf("Motorista a %.0fm\n", response);
            ssize_t bytes_received = recv(s, &response, sizeof(double), 0);
            if (bytes_received == -1)
            {
                logexit("Error receiving the response");
                exit(1);
            }
        }
        printf("O motorista chegou!\n");
        close(s);
        break;
    }

    

    return 0;
}