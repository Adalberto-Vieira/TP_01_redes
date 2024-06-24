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
#define BUFSZ 2048


int main(int argc, char **argv)
{
    int loop_control = 1;

    while (1){
        printf("0 - Sair\n");
        printf("1 - Senhor dos aneis\n");
        printf("2 - O poderose Chefao\n");
        printf("3 - Clube da luta\n");
        scanf("%d", &loop_control);
        
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }

        if (loop_control == 0){
            break;
        }

        int s;


        char buf[BUFSZ] = {0};

        struct sockaddr_storage storage;
        // Converte o endereço IP e a porta para uma estrutura de endereço de socket
        if (0 != addrparse(argv[2], argv[3], &storage)){
            logexit("error argparse");
            exit(EXIT_FAILURE);
        }
        // Cria um novo socket
        s = socket(storage.ss_family, SOCK_DGRAM, 0);

        if (s == SOCKET_ERR){
            logexit("socket");
            exit(EXIT_FAILURE);
        }

        sprintf(buf, "%d", loop_control);

        struct sockaddr *address = (struct sockaddr *)(&storage);

        int numbytes = sendto(s, buf, strlen(buf), 0, address, sizeof(storage));

        if (numbytes == -1) {
            perror("sendto");
            close(s);
            continue;
        }

        struct sockaddr_storage sockaddr_from;
        socklen_t from_len = sizeof(sockaddr_from);

        numbytes = recvfrom(s, buf, BUFSZ, 0, (struct sockaddr *) &sockaddr_from, &from_len);

        if (numbytes == -1) {
            perror("recvfrom");
            close(s);
            continue;
        }

        buf[numbytes] = '\0';
        buf[strcspn(buf, "\n")] = '\0';        

        while (strcmp(buf, "Ended")) {
            numbytes = recvfrom(s, buf, BUFSZ, 0, (struct sockaddr *) &sockaddr_from, &from_len);

            if (numbytes == -1) {
                perror("recvfrom");
                break;
            }

            buf[numbytes] = '\0';
            buf[strcspn(buf, "\n")] = '\0';

            if(strcmp(buf, "Ended"))
                printf(" %s\n", buf);
        }

        close(s);
    }

    return 0;
}