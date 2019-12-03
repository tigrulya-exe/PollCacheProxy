#pragma once

#include <vector>
#include <list>
#include <poll.h>
#include <map>
#include <netinet/in.h>
#include "cache.h"
#include "httpParser/httpParser.h"
#include "models/HttpMessage.h"
#include "models/Connection.h"


class Proxy{
private:
    static const int ACCEPT_INDEX = 0;
    static const int MAX_CONNECTIONS = 510;
    static const int BUF_SIZE = 32768;

    std::vector<pollfd> pollFds;
    std::vector<Connection> clientConnections;
    std::vector<Connection> serverConnections;
    // key - socket fd, value - offset
    std::map<int, int> cacheOffsets;
    // key - socket fd, value - error message
    std::map<int, char*> errors;

    cache cacheNodes;
    sockaddr_in serverAddress;
    int portToListen;

//    void atError(std::_List_iterator<Connection>&);
    int initProxySocket();
    void initAddress(sockaddr_in*, int);
    void addNewConnection(int);
    void checkClientsConnections();
    HttpMessage parseHttpRequest(Connection &client);

public:

    void start();

    Proxy(int, sockaddr_in);

    void checkRequest(HttpMessage &request);

    Connection initServerConnection(Connection &);

    void checkServerConnections();

    bool isNewPathRequest(Connection &clientConnection);

    bool checkSend(Connection &connection);

    void sendDataFromCache(Connection &connection);

    void sendRequestToServer(Connection &serverConnection);

    bool checkRecv(Connection &connection);

    void receiveData(Connection &connection, bool isServer);

    void checkServerResponse(const char* URL, char *response, int responseLength, int socketFd);

    bool isCorrectResponseStatus(const char* URL, char *response, int responseLength);

    bool checkForErrors(Connection &connection);

    void removeClient(Connection &connection);
};
