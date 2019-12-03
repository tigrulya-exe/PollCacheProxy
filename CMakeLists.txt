cmake_minimum_required(VERSION 3.15)
project(cacheProxy)

set(CMAKE_CXX_STANDARD 14)

add_executable(cacheProxy main.cpp httpParser/httpParser.c util/argResolver.cpp util/argResolver.h exceptions/wrongArgumentException.h exceptions/socketClosedException.h proxy.cpp proxy.h cache.cpp cache.h cmake-build-debug/serverContext.cpp cmake-build-debug/serverContext.h models/Connection.h models/HttpMessage.h exceptions/unsupportedMethodException.h)