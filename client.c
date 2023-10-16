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

struct action
{
    int type;
    int coordinates[2];
    int board[4][4];
};

int main(int argc, char **argv)
{
    int s;

    char addressStr[BUFSZ];

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage))
    {
        logexit("error argparse");
        exit(EXIT_FAILURE);
    }
    s = socket(storage.ss_family, SOCK_STREAM, 0);

    if (s == SOCKET_ERR)
    {
        logexit("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr *address = (struct sockaddr *)(&storage);

    if (CON_ERR != connect(s, address, sizeof(storage)))
    {
        logexit("connect");
        exit(EXIT_FAILURE);
    }

    addrtostr(address, addressStr, BUFSZ);

    while (1)
    {
        int x = 0, y = 0;
        struct action message;
        char message_type_str[30];
        if (scanf("%29s", message_type_str) == 1)
        {
            if (strcmp("start", message_type_str) == 0)
            {
                message.type = htons(0);
                ssize_t bytes_sent = send(s, &message, sizeof(struct action), 0);
                if (bytes_sent == -1)
                {
                    logexit("Error sending the message");
                    exit(1);
                }
            }
            else if (strcmp("reset", message_type_str) == 0)
            {
                message.type = htons(5);
                ssize_t bytes_sent = send(s, &message, sizeof(struct action), 0);
                if (bytes_sent == -1)
                {
                    logexit("Error sending the message");
                    exit(1);
                }
            }
            else if (strcmp("exit", message_type_str) == 0)
            {
                message.type = htons(7);
                ssize_t bytes_sent = send(s, &message, sizeof(struct action), 0);
                if (bytes_sent == -1)
                {
                    logexit("Error sending the message");
                    exit(1);
                }
                break;
            }
            else if (strcmp("reveal", message_type_str) == 0)
            {
                if (scanf("%d,%d", &x, &y) != 2)
                {
                    printf("error: command not found\n");
                    continue;
                }
                if ((x<0 || x>3)||(y<0 || y>3))
                {
                    printf("error: invalid cell\n");
                    continue;
                }
                else if(message.board[x][y] != -3 && message.board[x][y] != -2)
                {
                    printf("error: cell already revealed\n");
                    continue;
                }
                message.coordinates[0] = htons(x);
                message.coordinates[1] = htons(y);
                message.type = htons(1);
                ssize_t bytes_sent = send(s, &message, sizeof(struct action), 0);
                if (bytes_sent == -1)
                {
                    logexit("Error sending the message");
                    exit(1);
                }
            }
            else if (strcmp("flag", message_type_str) == 0)
            {
                if (scanf("%d,%d", &x, &y) != 2)
                {
                    printf("error: command not found\n");
                    continue;
                }
                if ((x<0 || x>3)||(y<0 || y>3))
                {
                    printf("error: invalid cell\n");
                    continue;
                }
                else if(message.board[x][y] == -3)
                {
                    printf("error: cell already has a flag\n");
                    continue;
                }
                else if(message.board[x][y] != -2)
                {
                    printf("error: cannot insert flag in revealed cell\n");
                    continue;
                }
                message.coordinates[0] = htons(x);
                message.coordinates[1] = htons(y);
                message.type = htons(2);
                ssize_t bytes_sent = send(s, &message, sizeof(struct action), 0);
                if (bytes_sent == -1)
                {
                    logexit("Error sending the message");
                    exit(1);
                }
            }
            else if (strcmp("remove_flag", message_type_str) == 0)
            {
                if (scanf("%d,%d", &x, &y) != 2)
                {
                    printf("error: command not found\n");
                    continue;
                }
                if ((x<0 || x>3)||(y<0 || y>3))
                {
                    printf("error: invalid cell\n");
                    continue;
                }
                message.coordinates[0] = htons(x);
                message.coordinates[1] = htons(y);
                message.type = htons(4);
                ssize_t bytes_sent = send(s, &message, sizeof(struct action), 0);
                if (bytes_sent == -1)
                {
                    logexit("Error sending the message");
                    exit(1);
                }
            }
            else
            {
                printf("error: command not found\n");
                continue;
            }
        }
        else
        {
            printf("error: command not found\n");
            continue;
        }

    

        struct action response;
        ssize_t bytes_received = recv(s, &response, sizeof(struct action), 0);
        if (bytes_received == -1)
        {
            logexit("Error receiving the response");
            exit(1);
        }

        response.type = ntohs(response.type);
        response.coordinates[0] = ntohs(response.coordinates[0]);
        response.coordinates[1] = ntohs(response.coordinates[1]);

        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                response.board[i][j] = ntohl(response.board[i][j]);
            }
        }

        if (response.type == 3)
        {
            print_matrix(response.board);
        }
        else if (response.type == 6)
        {
            printf("YOU WIN!\n");
            print_matrix(response.board);
        }
        else if (response.type == 8)
        {
            printf("GAME OVER!\n");
            print_matrix(response.board);
        }
        else
        {
            printf("Received an unexpected response type: %d\n", response.type);
        }
        message = response;
    }

    close(s);

    return 0;
}