#pragma once

#include <cstdio>
#include "../httpParser/httpParser.h"

struct HttpRequest {
    int version;

    size_t methodLen;

    size_t pathLen;

    const char *method;

    const char *path;

    phr_header headers[100];

    size_t headersCount;
};
