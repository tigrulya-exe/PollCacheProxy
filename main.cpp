#include <iostream>
#include <cstdio>
#include "proxy.h"
#include "exceptions/wrongArgumentException.h"
#include "util/argResolver.h"

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
        Proxy proxy(resolver.getPortToListen(), resolver.getServerAddress());
        // Proxy proxy(getPort(argv[1]), constructServerAddress(argv[2], getPort(argv[3])));
        proxy.start();
    } catch(WrongArgumentException exception){
        std::cout << exception.what() << std::endl;
        ArgResolver::printUsage();
    }
    catch(std::exception e){
        std::cout << "exc" << std::endl;
    }
}

//int main(){
//    int version;
//    size_t methodLen, pathLen;
//    const char *method, *path;
//    struct phr_header headers[100];
//    size_t num_headers = sizeof(headers) / sizeof(headers[0]);
//
//    char buf[18] = "GET / HTTP/1.1\n\n";
//
//    int reqSize = phr_parse_request(buf, 17, &method, &methodLen, &path, &pathLen,
//                                    &version, headers, &num_headers, 0);
//
//    std::cout << reqSize << std:: endl;
//}