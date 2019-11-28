#pragma once

#include "Context.h"
#include "ClientContext.h"

class ServerContext : public Context {
public:
    ServerContext(size_t pollFdsIndex, int socketFd, ClientContext &clientContext);

private:
    ClientContext& clientContext;
};


