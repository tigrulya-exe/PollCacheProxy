cmake_minimum_required(VERSION 3.15)
project(cacheProxy)

set(CMAKE_CXX_STANDARD 14)

add_executable(cacheProxy main.cpp httpParser/HttpParser.c util/ArgResolver.cpp util/ArgResolver.h exceptions/WrongArgumentException.h exceptions/SocketClosedException.h Proxy.cpp Proxy.h Cache.cpp Cache.h models/Connection.h models/HttpRequest.h exceptions/ProxyException.h constants.h)