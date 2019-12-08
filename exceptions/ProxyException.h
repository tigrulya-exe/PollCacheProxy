//
// Created by tigrulya on 12/8/19.
#include <exception>
#include <string>
#include <utility>

#ifndef PROXYEXCEPTION_H
#define PROXYEXCEPTION_H

class ProxyException : public std::exception{

public:
    const char* what() const noexcept override{
        return whatString.c_str();
    }

    explicit ProxyException(std::string errorMsg) : whatString(std::move(errorMsg)){}
private:

    const std::string whatString;
};


#endif
