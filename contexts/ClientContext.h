#pragma once

#include <vector>
#include "Context.h"
#include "ServerContext.h"

class ClientContext : public Context{
public:
    ClientContext(size_t pollFdsIndex, int socketFd, ServerContext &serverContext);

    const std::vector<char> &getData() const;

private:
    std::vector<char> data;
    ServerContext& serverContext;
};