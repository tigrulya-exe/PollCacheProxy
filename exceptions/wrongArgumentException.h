#pragma once

#include <exception>

class WrongArgumentException : public std::exception{
public:
    virtual const char* what() const noexcept{
        return "Wrong arguments";
    }
};