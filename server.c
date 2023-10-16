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
#define MATRIX_SIZE 4

struct action
{
    int type;
    int coordinates[2];
    int board[4][4];
};

int matrix_has_no_bombs(int matrix[MATRIX_SIZE][MATRIX_SIZE], int bombs[MATRIX_SIZE][MATRIX_SIZE]) {
    int total_bombs = 0;
    int detected_bombs = 0;
    int avaible = 0;
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            if(bombs[i][j] == 1){
                total_bombs += 1;
            }
            if (matrix[i][j] == -2){
                avaible += 1;
            }
            if(matrix[i][j] == -3){
                if(bombs[i][j] == 1){
                    detected_bombs += 1;
                }
            }
        }
    }

    if(total_bombs == (avaible+detected_bombs)){
        
        return 1; 
    }
    return 0;
}

void read_matrix_from_file(const char *filename, int matrix[MATRIX_SIZE][MATRIX_SIZE], int bombs[MATRIX_SIZE][MATRIX_SIZE])
{
    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("Reading matrix from file: %s\n", filename);
        perror("Error opening the file");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            bombs[i][j] = 0;
            if (fscanf(file, "%d,", &matrix[i][j]) != 1)
            {
                perror("Error reading from file");
                exit(1);
            }

            if (matrix[i][j] == -1)
            {
                bombs[i][j] = 1;
            }
        }
    }

    fclose(file);
}

int main(int argc, char **argv)
{
    const char *input_filename = argv[4];
    int bombs[MATRIX_SIZE][MATRIX_SIZE];
    int board[MATRIX_SIZE][MATRIX_SIZE];
    int board_current_game[MATRIX_SIZE][MATRIX_SIZE];

    read_matrix_from_file(input_filename, board, bombs);
    print_matrix(board);

    int s;
    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage))
    {
        logexit("error argparse");
        exit(EXIT_FAILURE);
    }
    s = socket(storage.ss_family, SOCK_STREAM, 0);

    if (s == -1)
    {
        logexit("socket");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0)
    {
        logexit("error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr *address = (struct sockaddr *)(&storage);

    if (0 != bind(s, address, sizeof(storage)))
    {
        logexit("bind");
        exit(EXIT_FAILURE);
    }

    if (0 != listen(s, 1))
    {
        logexit("listen");
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        struct sockaddr_storage cstorage;
        struct sockaddr *client_sockaddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int client_sock = accept(s, client_sockaddr, &caddrlen);
        if (client_sock == -1)
        {
            logexit("conn error");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("client connected\n");
        }
        while (1)
        {
            struct action message;
            struct action response;
            size_t count = recv(client_sock, &message, sizeof(struct action), 0);
            message.type = ntohs(message.type);
            message.coordinates[0] = ntohs(message.coordinates[0]);
            message.coordinates[1] = ntohs(message.coordinates[1]);
            

            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    message.board[i][j] = ntohl(message.board[i][j]);
                }
            }
            if (count == -1)
            {
                break;
            }
            else if (count == 0)
            {
                printf("client disconnected\n");
                close(client_sock);
            }

            if (message.type == 0)
            {
                response.type = 3;
                for (int i = 0; i < MATRIX_SIZE; i++)
                {
                    for (int j = 0; j < MATRIX_SIZE; j++)
                    {
                        board_current_game[i][j] = -2;
                        response.board[i][j] = board_current_game[i][j];
                    }
                }
                
            }
            else if (message.type == 1)
            {
                int x = message.coordinates[0];
                int y = message.coordinates[1];
                if (bombs[x][y])
                {

                    response.type = 8;
                    for (int i = 0; i < MATRIX_SIZE; i++)
                    {
                        for (int j = 0; j < MATRIX_SIZE; j++)
                        {
                            response.board[i][j] = board[i][j];
                        }
                    }
                }
                else
                {
                    board_current_game[x][y] = board[x][y];
                    if (matrix_has_no_bombs(board_current_game, bombs))
                    {
                        response.type = 6;
                        for (int i = 0; i < MATRIX_SIZE; i++)
                        {
                            for (int j = 0; j < MATRIX_SIZE; j++)
                            {
                                response.board[i][j] = board[i][j];
                            }
                        }
                    }
                    else
                    {

                        response.type = 3;
                        for (int i = 0; i < MATRIX_SIZE; i++)
                        {
                            for (int j = 0; j < MATRIX_SIZE; j++)
                            {
                                response.board[i][j] = board_current_game[i][j];
                            }
                        }
                    }
                }
                
            }
            else if (message.type == 2)
            {
                int x = message.coordinates[0];
                int y = message.coordinates[1];

                board_current_game[x][y] = -3;
                if (matrix_has_no_bombs(board_current_game, bombs))
                {
                    response.type = 6;
                    for (int i = 0; i < MATRIX_SIZE; i++)
                    {
                        for (int j = 0; j < MATRIX_SIZE; j++)
                        {
                            response.board[i][j] = board[i][j];
                        }
                    }
                }
                else
                {
                    response.type = 3;
                    for (int i = 0; i < MATRIX_SIZE; i++)
                    {
                        for (int j = 0; j < MATRIX_SIZE; j++)
                        {
                            response.board[i][j] = board_current_game[i][j];
                        }
                    }
                    
                }
            }
            else if (message.type == 4)
            {
                int x = message.coordinates[0];
                int y = message.coordinates[1];

                board_current_game[x][y] = -2;
                response.type = 3;
                for (int i = 0; i < MATRIX_SIZE; i++)
                {
                    for (int j = 0; j < MATRIX_SIZE; j++)
                    {
                        response.board[i][j] = board_current_game[i][j];
                    }
                }
            }
            else if (message.type == 5)
            {
                read_matrix_from_file(input_filename, board, bombs);
                response.type = 3;
                for (int i = 0; i < MATRIX_SIZE; i++)
                {
                    for (int j = 0; j < MATRIX_SIZE; j++)
                    {
                        board_current_game[i][j] = -2;
                        response.board[i][j] = board_current_game[i][j];
                    }
                }
                printf("starting new game\n");
            }
            else if (message.type == 7)
            {
                printf("client disconnected\n");
                close(client_sock);

            }
            else
            {
                printf("Received a message with an unexpected type: %d\n", message.type);
                close(client_sock);
                break;
            }
            response.type = htons(response.type);
            response.coordinates[0] = htons(response.coordinates[0]);
            response.coordinates[1] = htons(response.coordinates[1]);
            
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    response.board[i][j] = htonl(response.board[i][j]);
                }
            }
            send(client_sock, &response, sizeof(struct action), 0);
        }
    }

    exit(EXIT_SUCCESS);
}