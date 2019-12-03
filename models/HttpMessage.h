#pragma once

struct HttpMessage{
    const char *method;

    const char *path;

    int version;

    HttpMessage(const char *method, const char *path, int version) : method(method), path(path), version(version) {}
};