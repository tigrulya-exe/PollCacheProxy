#include <iostream>
#include "Proxy.h"
#include "exceptions/WrongArgumentException.h"
#include "util/ArgResolver.h"

int main(int argc, char* argv[]){
    try{
        Proxy proxy(ArgResolver::getPortToListen(argc, argv));
        proxy.start();
    } catch(WrongArgumentException& exception){
        std::cout << exception.what() << std::endl;
        ArgResolver::printUsage();
    }
    catch(std::exception& exception){
        std::cout << exception.what() << std::endl;
    }
}