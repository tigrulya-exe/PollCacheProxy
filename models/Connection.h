#pragma once

#include <cstdio>

struct Connection {

    Connection(int socketFd, int pollFdsIndex) : pollFdsIndex(pollFdsIndex), socketFd(socketFd) {}

    void initBuffer(){
        buffer = new std::vector<char>();
    }

    int pollFdsIndex;

    int socketFd;

    const char* URl;

    std::vector<char>* buffer;
};