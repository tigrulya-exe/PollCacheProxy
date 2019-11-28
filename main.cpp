#include <sys/socket.h>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "proxy.h"
#include "wrongArgumentException.h"
#include "argResolver.h"

// sockaddr_in constructServerAddress(char* address, int port){
//     sockaddr_in serverAddress;
//     serverAddress.sin_family = AF_INET;
//     serverAddress.sin_port = htons(port);
//     if(inet_pton(AF_INET, address, &serverAddress.sin_addr) <= 0){
//         throw WrongArgumentException();
//     }

//     return serverAddress;
// }

// int getPort(char* portStr){
//     char errorFlag;

//     int portToReturn;
//     if (sscanf(portStr, "%d%c\n", &portToReturn, &errorFlag) != 1){
//         throw WrongArgumentException();
//     }

//     return portToReturn;
// }

int main(int argc, char* argv[]){
    try{
        ArgResolver resolver{};
        resolver.resolve(argc, argv);
        TcpProxy proxy(resolver.getPortToListen(), resolver.getServerAddress());
        // TcpProxy proxy(getPort(argv[1]), constructServerAddress(argv[2], getPort(argv[3])));
        proxy.start();
    } catch(WrongArgumentException exception){
        std::cout << exception.what() << std::endl;
        ArgResolver::printUsage();
    }
    catch(std::exception e){
        std::cout << "exc" << std::endl;
    }
}