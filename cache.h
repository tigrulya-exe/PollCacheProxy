#pragma once

#include <vector>
#include <map>
#include "connection.h"

class cache{
private:
    std::map<std::string, std::vector<char>> cacheMap;
public:
    void addCacheClient(std::string url, ConnectionContext& client);
    bool containsUrl(std::string url);
    // maybe replace with vector
    void addData(std::string url, char* newData);
    // very todo + tmp ))
    void removeCacheClient(std::string url, ConnectionContext client);
    std::vector<char> getCurrentData(std::string url);
};