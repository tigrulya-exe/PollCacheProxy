#pragma once

#include <vector>
#include <list>
#include <poll.h>
#include <map>
#include <netinet/in.h>
#include "connection.h"
#include "cache.h"
#include "httpParser/httpParser.h"
#include "contexts/ClientContext.h"


class TcpProxy{
private:
    static const int ACCEPT_INDEX = 0;
    static const int MAX_CONNECTIONS = 510;

    std::vector<pollfd> pollFds;
    std::vector<ClientContext> clientContexts;
    std::vector<ServerContext> serverContexts;
//    std::list<Connection> connections;
//    std::map<std::string, ConnectionContext> serverContexts;
//    std::map<std::string, std::vector<ConnectionContext>> cacheClients;
//    std::map<ConnectionContext, std::string> clientUrls;

    cache cache;
    sockaddr_in serverAddress;
    int portToListen;

//    void checkClient(ConnectionContext&, ConnectionContext&);
    void atError(std::_List_iterator<Connection>&);
    int initProxySocket();
    void initAddress(sockaddr_in*, int);
    void checkConnections();
    void removePollFd(int, int);
    void checkPollFdIndex(ConnectionContext& ,int);
    void addNewConnection(int);
    bool checkPollFd(pollfd&, ConnectionContext&, ConnectionContext&);
//    bool checkRecv(ConnectionContext&, pollfd&);
    void checkSend(ConnectionContext&, ConnectionContext&, pollfd&);
    void parseHttpRequest(ConnectionContext& );
    void checkUrl(char* ,ConnectionContext& );
    void closeSockets();
    void checkClients();
public:
    void start();

    TcpProxy(int, sockaddr_in);

    void checkClient(const ConnectionContext &clientContext);

    bool checkRecv(ConnectionContext &recvContext);

};
