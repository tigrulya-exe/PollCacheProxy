#pragma once

#include <cstdio>

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