#include <string>

using std::string;

#ifndef CONSTANT_H
#define CONSTANT_H

namespace{
    namespace errors {
        const string BAD_REQUEST = "HTTP/1.0 400\r\n\r\nBAD REQUEST\r\n";
        const string NOT_IMPLEMENTED = "HTTP/1.0 400\r\n\r\nBAD REQUEST\r\n";
        const string SERVER_CONNECT = "HTTP/1.0 500\r\n\r\nERROR CONNECTING WITH SERVER\r\n";
        const string CACHE_SEND_ERROR = "HTTP/1.0 500\r\n\r\nERROR SENDING DATA FROM CACHE\r\n";
        const string INTERNAL_ERROR = "HTTP/1.0 500\r\n\r\nINTERNAL PROXY ERROR\r\n";
    }
}

#endif
