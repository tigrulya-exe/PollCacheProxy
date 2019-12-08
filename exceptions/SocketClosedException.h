#include <exception>

#ifndef SOCKET_CLOSE_EXCEPTION_H
#define SOCKET_CLOSE_EXCEPTION_H
class SocketClosedException : public std::exception{
public:
    virtual const char* what() const noexcept{
        return "Socket closed";
    }
};

#endif