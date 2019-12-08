#include <netinet/in.h>

#ifndef ARG_RECOLVER_H
#define ARG_RECOLVER_H
class ArgResolver{
private:
    static const int LISTEN_PORT_INDEX = 1;

    static const int ARGS_COUNT = 2;

public:
    static int getPortToListen(int, char**);
    static void printUsage();
};

#endif
