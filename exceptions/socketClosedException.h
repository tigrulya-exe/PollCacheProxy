#pragma once

#include <exception>

class SocketClosedException : public std::exception{
public:
    virtual const char* what() const noexcept{
        return "Socket closed";
    }
};