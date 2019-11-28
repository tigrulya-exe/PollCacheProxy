#pragma once

#include <cstdio>

class Context {
public:
    size_t getPollFdsIndex() const;

    int getSocketFd() const;

    Context(size_t pollFdsIndex, int socketFd);
private:
    size_t pollFdsIndex;
    int socketFd;
};