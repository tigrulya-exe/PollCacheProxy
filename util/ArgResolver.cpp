#include <iostream>
#include "ArgResolver.h"
#include "../exceptions/WrongArgumentException.h"

void ArgResolver::printUsage(){
    std::cout << "proxyExecutable <port_to_listen>" << std::endl;
}

int ArgResolver::getPortToListen(int argc, char ** argv){
    if(argc != ARGS_COUNT){
        throw WrongArgumentException();
    }

    char errorFlag;

    int portToReturn;
    if (sscanf(argv[LISTEN_PORT_INDEX], "%d%c\n", &portToReturn, &errorFlag) != 1){
        throw WrongArgumentException();
    }

    return portToReturn;
}