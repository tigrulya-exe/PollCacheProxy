#include <iostream>
#include "proxy.h"
#include "exceptions/wrongArgumentException.h"
#include "util/argResolver.h"

int main(int argc, char* argv[]){
    try{
        ArgResolver resolver{};
        resolver.resolve(argc, argv);
        Proxy proxy(resolver.getPortToListen(), resolver.getServerAddress());
        proxy.start();
    } catch(WrongArgumentException& exception){
        std::cout << exception.what() << std::endl;
        ArgResolver::printUsage();
    }
    catch(std::exception& exception){
        std::cout << exception.what() << std::endl;
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