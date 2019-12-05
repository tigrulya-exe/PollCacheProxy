//
// Created by tigrulya on 11/28/19.
//

#pragma once

#pragma once
#include <netinet/in.h>


class ArgResolver{
private:
    static const int LISTEN_PORT_INDEX = 1;

    static const int ARGS_COUNT = 2;

public:
    static int getPortToListen(int, char**);
    static void printUsage();
};
