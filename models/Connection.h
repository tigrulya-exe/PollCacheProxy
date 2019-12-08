#include <cstdio>

#ifndef CONNECTION_H
#define CONNECTION_H

struct Connection {

    Connection(int socketFd, int pollFdsIndex) : pollFdsIndex(pollFdsIndex), socketFd(socketFd) {}

    void initBuffer(){
        buffer = new std::vector<char>();
    }

    int pollFdsIndex;

    int socketFd;

    std::string URl;

    std::vector<char>* buffer;
};

#endif
