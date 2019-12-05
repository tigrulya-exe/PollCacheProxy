#pragma once

#include <cstdio>
#include "../httpParser/httpParser.h"

struct HttpRequest {
    int version;

    std::string method;

    std::string path;

    phr_header headers[100];

    std::string host;

    size_t headersCount;
};
