//
// Created by tigrulya on 11/28/19.
//

#include "cache.h"

bool cache::contains(std::string url) {
    return cacheMap.find(url) != cacheMap.end();
}

std::vector<char> &cache::getCurrentData(const std::string& url) {
    return cacheMap[url];
}

void cache::addData(std::string url, char *newData, int newDataLength) {
    auto& cacheNode = cacheMap[url];
    cacheNode.insert(cacheNode.end(), newData, newData + newDataLength);
}

bool cache::cacheNodeReady(const char* url) {
    if(url == nullptr || !contains(url)){
        return true;
    }

    return cacheReady[url];
}

void cache::setNodeReady(const std::string &url, bool isReady) {
    cacheReady[url] = isReady;
}
