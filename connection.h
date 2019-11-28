#pragma once
#include <iostream>

#define BUF_SIZE 94096

struct ConnectionContext{
    size_t pollFdsIndex;
    int socketFd;
    int bufLength;
    char data[BUF_SIZE];


    ConnectionContext(int fd, size_t pollFdsIndex) : socketFd(fd), pollFdsIndex(pollFdsIndex){}
};

struct ServerContext{
};

struct Connection{
    ConnectionContext clientContext;
    ConnectionContext serverContext;

    Connection(ConnectionContext clientContext, ConnectionContext serverContext) :
            clientContext(clientContext), serverContext(serverContext) {}
};