//
// Created by tigrulya on 11/29/19.
//

#include "ClientContext.h"

ClientContext::ClientContext(size_t pollFdsIndex, int socketFd, ServerContext &serverContext): Context(pollFdsIndex,
                                                                                                        socketFd),
                                                                                                serverContext(
                                                                                                        serverContext) {}

const std::vector<char> &ClientContext::getData() const {
    return data;
}
