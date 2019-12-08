#include <cstdio>
#include "../httpParser/HttpParser.h"

#ifndef REQUEST_H
#define REQUEST_H

struct HttpRequest {
    int version;

    std::string method;

    std::string path;

    phr_header headers[100];

    std::string host;

    size_t headersCount;
};
#endif
