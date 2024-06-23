#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>



// Função para imprimir mensagem de erro e encerrar o programa
void logexit(const char *error_msg)
{
    perror(error_msg);
    exit(EXIT_FAILURE);
}

// Função para analisar e converter um endereço IP e uma porta para uma estrutura de endereço de socket
int addrparse(
    const char *addrstring,// Endereço IP
    const char *portstr, // Porta
    struct sockaddr_storage *storage) // Armazenamento para a estrutura de endereço
{
    // Verifica se o endereço IP ou a porta são nulos
    if (addrstring == NULL || portstr == NULL)
    {
        return -1;
    }
    // Converte a porta de uma string para um número inteiro de 16 bits
    uint16_t port = (uint16_t)atoi(portstr);
    // Verifica se a porta é válida
    if (port == 0)
    {
        return -1;
    }
    // Converte a porta para o formato de rede
    port = htons(port);

    // Inicializa uma estrutura de endereço IPv4
    struct in_addr inaddr4;
    if (inet_pton(AF_INET, addrstring, &inaddr4))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    // Inicializa uma estrutura de endereço IPv6
    struct in_addr inaddr6;
    if (inet_pton(AF_INET6, addrstring, &inaddr6))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

// Função para converter uma estrutura de endereço de socket em uma string
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize)
{
    int version;
    uint16_t port;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";

    // Verifica a versão do endereço (IPv4 ou IPv6)
    if (addr->sa_family == AF_INET)
    {
        version = 4;
        // Converte o tipo de estrutura de endereço de socket para IPv4
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        // Converte o endereço IP para uma string legível
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1))
        {
            logexit("ntop");
        }
        // Converte a porta de rede para o formato de host
        port = ntohs(addr4->sin_port);
    }
    else if (addr->sa_family == AF_INET6)
    {
        version = 6;
        // Converte o tipo de estrutura de endereço de socket para IPv6
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        // Converte o endereço IP para uma string legível
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1))
        {
            logexit("ntop");
        }
        // Converte a porta de rede para o formato de host
        port = ntohs(addr6->sin6_port);
    }
    else
    {
        logexit("unknown protocol");
    }
    // Formata os dados em uma string
    if (str)
    {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

// Função para inicializar uma estrutura de endereço de socket para o servidor
int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage)
{
    // Converte a string de porta para um número inteiro de 16 bits
    uint16_t port = (uint16_t)atoi(portstr);
    // Verifica se a porta é válida
    if (port == 0)
    {
        return -1;
    }
    // Converte a porta para o formato de rede
    port = htons(port);
    // Limpa a estrutura de armazenamento
    memset(storage, 0, sizeof(*storage));
    // Verifica o protocolo e inicializa a estrutura de acordo com o tipo de endereço
    if(0 == strcmp(proto, "ipv4")){
        // Inicializa a estrutura de endereço para IPv4
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET; // Define a família do endereço como IPv4
        addr4->sin_port = port; // Define a porta do endereço
        addr4->sin_addr.s_addr = INADDR_ANY; // Define o endereço IP como qualquer interface disponível
        return 0;
    }else if (0 == strcmp(proto, "ipv6")){
        // Inicializa a estrutura de endereço para IPv6
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6; // Define a família do endereço como IPv6
        addr6->sin6_port = port; // Define a porta do endereço
        return 0;
    }else {
        return -1;
    }
}