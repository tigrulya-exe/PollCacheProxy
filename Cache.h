#include <vector>
#include <map>

#ifndef CACHE_H
#define CACHE_H

class Cache{
private:
    std::map<std::string, bool> cacheReady;

    std::map<std::string, std::vector<char>> cacheMap;
public:
    bool contains(std::string url);

    std::vector<char>& getCurrentData(const std::string& url);

    void addData(std::string& url, char *newData, int newDataLength);

    bool cacheNodeReady(std::string& url);

    void setNodeReady(const std::string& url, bool isReady);
};

#endif