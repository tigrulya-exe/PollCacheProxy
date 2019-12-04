#pragma once

#include <vector>
#include <list>
#include <poll.h>
#include <map>
#include <netinet/in.h>
#include "Cache.h"
#include "httpParser/httpParser.h"
#include "models/Connection.h"
#include "models/HttpRequest.h"


class Proxy{

public:
    void start();

    Proxy(int, sockaddr_in);

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

    Cache cacheNodes;
    sockaddr_in serverAddress;
    int portToListen;

    int initProxySocket();

    void initAddress(sockaddr_in*, int);

    void addNewConnection(int);

    void checkClientsConnections();

    HttpRequest parseHttpRequest(Connection &client);

    void removeServerConnection(Connection& connection);

    Connection initServerConnection(Connection &);

    void checkServerConnections();

    bool isNewPathRequest(Connection &clientConnection);

    bool checkSend(Connection &connection);

    void sendDataFromCache(Connection &connection);

    void sendRequestToServer(Connection &serverConnection);

    bool checkRecv(Connection &connection);

    int receiveData(Connection &connection, char* buf);

    void checkServerResponse(const char* URL, char *response, int responseLength);

    bool isCorrectResponseStatus(const char* URL, char *response, int responseLength);

    bool checkForErrors(Connection &connection);

    void removeClientConnection(Connection &connection);

    void notifyClientsAboutError(const char *URL, const char *error);

    void prepareClientsToWrite(const char *URL);

    void receiveDataFromServer(Connection &connection);

    void receiveDataFromClient(Connection &connection);

    void handleClientDataReceive(Connection &clientConnection);

    void checkRequest(HttpRequest &request);

    void removeConnection(Connection &connection);
};
