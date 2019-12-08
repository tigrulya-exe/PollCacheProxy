#include <exception>

#ifndef WRONG_ARGUMENT_EXCEPTION_H
#define WRONG_ARGUMENT_EXCEPTION_H
class WrongArgumentException : public std::exception{
public:
    virtual const char* what() const noexcept{
        return "Wrong arguments";
    }
};

#endif