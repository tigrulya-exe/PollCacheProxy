//
// Created by tigrulya on 11/29/19.
//

#include "ServerContext.h"

ServerContext::ServerContext(size_t pollFdsIndex, int socketFd, ClientContext &clientContext) : Context(pollFdsIndex,
                                                                                                        socketFd),
                                                                                                clientContext(
                                                                                                        clientContext) {}
