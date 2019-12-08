#include <vector>
#include <list>
#include <poll.h>
#include <map>
#include <netinet/in.h>
#include "Cache.h"
#include "httpParser/HttpParser.h"
#include "models/Connection.h"
#include "models/HttpRequest.h"
#include "constants.h"

#ifndef PROXY_H
#define PROXY_H

using ConnectionIter = __gnu_cxx::__normal_iterator<Connection *, std::vector<Connection>>;

class Proxy{

public:
    void start();

    explicit Proxy(int);

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
    std::map<int, std::string> errors;

    Cache cacheNodes;
    int portToListen;

    int initProxySocket();

    void initAddress(sockaddr_in*, int);

    void addNewConnection(int);

    void checkClientsConnections();

    HttpRequest parseHttpRequest(Connection &client);

    ConnectionIter removeServerConnection(Connection& connection);

    Connection initServerConnection(Connection &, HttpRequest& request);

    void checkServerConnections();

    bool checkSend(Connection &connection);

    bool sendDataFromCache(Connection &connection, bool cacheNodeReady);

    void sendRequestToServer(Connection &serverConnection);

    bool checkRecv(Connection &connection);

    int receiveData(Connection &connection, char* buf);

    bool isCorrectResponseStatus(char *response, int responseLength);

    bool checkForErrors(Connection &connection);

    ConnectionIter removeClientConnection(Connection &connection);

    void notifyClientsAboutError(std::string& URL, const char *error);

    void prepareClientsToWrite(std::string& URL);

    void receiveDataFromServer(Connection &connection);

    bool receiveDataFromClient(Connection &connection);

    void handleClientDataReceive(Connection &clientConnection);

    void checkRequest(HttpRequest &request);

    void removeConnection(Connection &connection);

    sockaddr_in getServerAddress(const char *host);

    void checkIfError(Connection &connection);

    bool allDataHasBeenSent(Connection &clientConnection);

    void stop();
};

#endif
