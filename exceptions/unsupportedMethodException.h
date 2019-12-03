#pragma once

#include <bits/exception.h>

class unsupportedMethodException : std::exception {
public:
    virtual const char* what() const noexcept{
        return "Unsupported method";
    }
};
