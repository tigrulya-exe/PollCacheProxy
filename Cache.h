#pragma once

#include <vector>
#include <map>

class Cache{
private:
    std::map<std::string, bool> cacheReady;

    std::map<std::string, std::vector<char>> cacheMap;
public:
    bool contains(std::string url);

    std::vector<char>& getCurrentData(const std::string& url);

    void addData(std::string url, char *newData, int newDataLength);

    bool cacheNodeReady(const char* url);

    void setNodeReady(const std::string& url, bool isReady);
};