#pragma once
#include <stdlib.h>
#include <stdio.h>

#include <arpa/inet.h>

#define MATRIX_SIZE 4

void logexit(const char *error_msg);

int addrparse(
    const char *addrstring,
    const char *portstr,
    struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);