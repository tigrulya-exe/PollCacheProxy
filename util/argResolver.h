//
// Created by tigrulya on 11/28/19.
//

#pragma once

#pragma once
#include <netinet/in.h>


class ArgResolver{
private:
    static const int LISTEN_PORT_INDEX = 1;
    static const int SERVER_ADDR_INDEX = 2;
    static const int SERVER_PORT_INDEX = 3;

    static const int ARGS_COUNT = 4;
    int portToListen;
    sockaddr_in serverAddress;

    void constructServerAddress(char*, int);
    int getPort(char*);

public:
    void resolve(int, char**);
    int getPortToListen();
    sockaddr_in getServerAddress();
    static void printUsage();
};
