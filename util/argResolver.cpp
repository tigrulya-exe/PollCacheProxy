#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <cstring>
#include "argResolver.h"
#include "../exceptions/wrongArgumentException.h"

// for solarka

// void ArgResolver::resolve(int argc, char* argv[]){
//     if(argc != ARGS_COUNT){
//         throw WrongArgumentException();
//     }

//     int errNum;
//     hostent* serverHostEntity = getipnodebyname(argv[SERVER_ADDR_INDEX], AF_INET, 0, &errNum);

//     if(serverHostEntity == NULL){
//         perror("Wrong hostname");
//         throw WrongArgumentException();
//     }

//     portToListen = getPort(argv[LISTEN_PORT_INDEX]);
//     serverAddress.sin_family = AF_INET;
//     serverAddress.sin_port = htons(getPort(argv[SERVER_PORT_INDEX]));
//     memcpy(&(serverAddress.sin_addr.s_addr), serverHostEntity->h_addr_list[0], sizeof(in_addr));

//     // if(inet_pton(AF_INET, address, &serverAddress.sin_addr) <= 0){
//     //     throw WrongArgumentException();
//     // }

//     freehostent(serverHostEntity);
// }

// for linux

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