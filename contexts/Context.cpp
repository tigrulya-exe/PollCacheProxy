//
// Created by tigrulya on 11/29/19.
//

#include "Context.h"

Context::Context(size_t pollFdsIndex, int socketFd) : pollFdsIndex(pollFdsIndex), socketFd(socketFd) {}

size_t Context::getPollFdsIndex() const {
    return pollFdsIndex;
}

int Context::getSocketFd() const {
    return socketFd;
}
