#include <sys/socket.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <poll.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include "proxy.h"
#include "httpParser/httpParser.h"
#include "exceptions/socketClosedException.h"

// один из сценариев тестирования прокси когда страница частично докачана, потом этот урл 
// запрашивает второй клиент, то если у него соединение с сервером всего 10 бит в секунду 
// он должен быстро получить ту часть данных, которая лежит в кеше, а потом получать  
// с дефолтной скоростью остальные данные

// вынести кеш в отдельный класс - со всеми операциями по обращению к данным

TcpProxy::TcpProxy(int portToListen, sockaddr_in serverAddress) : portToListen(portToListen), serverAddress(serverAddress){}

void TcpProxy::start(){
    int sockFd = initProxySocket();
    int pollState;

    pollFds.push_back({sockFd, POLLIN, 0});

    while (true){
        if((pollState = poll(pollFds.data(), pollFds.size(), 0)) < 0){
            perror("Poll");
            exit(EXIT_FAILURE);
        }

        if(!pollState){
            continue;
        }

        if(pollFds[ACCEPT_INDEX].revents & POLLIN){
            addNewConnection(sockFd);
        }

        checkConnections();
    }

}

//void TcpProxy::checkUrl(char* url, ConnectionContext& client){
//    auto cacheBuf = cache.find(url);
//
//    if(cacheBuf == cache.end()){
//        cache[url] = std::vector<char>();
//    }
//
//    auto urlClients = cacheClients.find(url);
//    if(urlClients == cacheClients.end()){
//        cacheClients[url] = std::vector<ConnectionContext>{client};
//    }
//}
//
//void TcpProxy::parseHttpRequest(ConnectionContext& client){
//    int version;
//    size_t methodLen, pathLen;
//    const char *method, *path;
//    struct phr_header headers[100];
//    size_t num_headers = sizeof(headers) / sizeof(headers[0]);
//
//    int reqSize = phr_parse_request(client.data, client.bufLength, &method, &methodLen, &path, &pathLen,
//                                    &version, headers, &num_headers, 0);
//
//    char url[pathLen];
//    memcpy(url, path, pathLen);
//    checkUrl(url, client);
//
//    for(auto subsVec : cacheClients){
//        for(auto client : subsVec.second){
//            std::cout << client.socketFd << std::endl;
//        }
//    }
//
//    // printf("request is %d bytes long\n", reqSize);
//    // printf("method is %.*s\n", (int)methodLen, method);
//    // printf("path is %.*s\n", (int)pathLen, path);
//    // printf("HTTP version is 1.%d\n", version);
//    // printf("headers:\n");
//    // for (int i = 0; i != num_headers; ++i) {
//    //     printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name,
//    //     (int)headers[i].value_len, headers[i].value);
//    // }
//}

void TcpProxy::initAddress(sockaddr_in* addr, int port){
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(port);
}

int TcpProxy::initProxySocket(){
    struct sockaddr_in address;
    initAddress(&address, portToListen);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(sockfd, MAX_CONNECTIONS) < 0){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void atExit(char* msg){
    perror(msg);
    exit(EXIT_FAILURE);
}
//
//void  TcpProxy::closeSockets(){
//    for(auto& connection : connections){
//        if(close(connection.clientContext.socketFd) < 0){
//            atExit("Error closing socket");
//        }
//
//        if(close(connection.serverContext.socketFd) < 0){
//            atExit("Error closing socket");
//        }
//    }
//}

void TcpProxy::atError(std::_List_iterator<Connection>& connection){
    if(close(connection->clientContext.socketFd) < 0 ){
        atExit("Error closing socket");
    }
    if(close(connection->serverContext.socketFd) < 0){
        atExit("Error closing socket");
    }

    removePollFd(connection->clientContext.pollFdsIndex, connection->serverContext.pollFdsIndex);
//    connection = connections.erase(connection);
}

//void TcpProxy::checkConnections(){
//    int i = 0;
//    bool isRecvedFromClient, isRecvedFromServer;
//    for(auto connection = connections.begin(); connection != connections.end(); ){
//        try{
//            ConnectionContext& clientContext = connection->clientContext;
//            ConnectionContext& serverContext = connection->serverContext;
//
//            isRecvedFromClient = checkPollFd(pollFds[clientContext.pollFdsIndex], clientContext, serverContext);
//            isRecvedFromServer = checkPollFd(pollFds[serverContext.pollFdsIndex], serverContext, clientContext);
//
//            if(isRecvedFromClient){
//                parseHttpRequest(clientContext);
//                pollFds[serverContext.pollFdsIndex].events = POLLOUT;
//            }
//
//            if(isRecvedFromServer){
//                pollFds[clientContext.pollFdsIndex].events = POLLOUT;
//            }
//
//            ++connection;
//        } catch(SocketClosedException& exc){
//            std::cout << exc.what() << std::endl;
//            atError(connection);
//        }
//    }
//}

void TcpProxy::checkClients() {
    bool dataSent = false;

    for (auto iter = clientUrls.begin(); iter != clientUrls.end(); ++iter) {
        checkRecv(iter->first);
    }
}

bool TcpProxy::checkRecv(ConnectionContext& recvContext){
    bool toReturn = false;
    pollfd& recvPollFd = pollFds[recvContext.pollFdsIndex];

    if(recvPollFd.revents & POLLIN){
        if((recvContext.bufLength = recv(recvContext.socketFd, (void*) recvContext.data, BUF_SIZE, 0)) < 0){
            perror("Error receiving data from client");
            return toReturn;
        }

        if(recvContext.bufLength == 0){
            throw SocketClosedException();
        }

        toReturn = true;
    }

    recvPollFd.revents = 0;
    return toReturn;
}

void TcpProxy::checkClient(const ConnectionContext& clientContext){
    bool isRecvedFromClient = checkPollFd(pollFds[clientContext.pollFdsIndex], clientContext, serverContext);

    if(isRecvedFromClient){
        parseHttpRequest(clientContext);
        pollFds[serverContext.pollFdsIndex].events = POLLOUT;
    }
}

void TcpProxy::checkConnections(){
    checkClients();
}

void TcpProxy::checkClient(ConnectionContext& clientContext, ConnectionContext& serverContext){
    bool isRecvedFromClient = checkPollFd(pollFds[clientContext.pollFdsIndex], clientContext, serverContext);

    if(isRecvedFromClient){
        parseHttpRequest(clientContext);
        pollFds[serverContext.pollFdsIndex].events = POLLOUT;
    }
}

void TcpProxy::removePollFd(int clientIndex, int serverIndex){

    for(auto& connection : connections){
        checkPollFdIndex(connection.clientContext, clientIndex);
        checkPollFdIndex(connection.clientContext, serverIndex);
        checkPollFdIndex(connection.serverContext, clientIndex);
        checkPollFdIndex(connection.serverContext, serverIndex);
    }

    pollFds.erase(pollFds.begin() + clientIndex);
    pollFds.erase(pollFds.begin() + serverIndex - 1);
}

void TcpProxy::checkPollFdIndex(ConnectionContext& connectionContext, int index){
    if(connectionContext.pollFdsIndex >= index){
        --connectionContext.pollFdsIndex;
    }
}

void TcpProxy::addNewConnection(int sockFd){
    int newSocketFd = accept(sockFd, NULL, NULL);

    if(newSocketFd < 0){
        perror("Error accepting connection");
        return;
    }

    int pollFdsSize = pollFds.size();
    clientUrls.emplace(new ConnectionContext(sockFd, pollFdsSize + 1), NULL);
    pollFds.push_back({newSocketFd, POLLIN, 0});

//    int serverSockFd = socket(AF_INET, SOCK_STREAM, 0);

//    if(serverSockFd < 0){
//        perror("Error creating socket");
//        if(close(newSocketFd) < 0){
//            atExit("Error closing socket");
//        }
//        return;
//    }

//    if(connect(serverSockFd, (struct sockaddr *)& serverAddress, sizeof(serverAddress)) == -1){
//        perror("Error connecting to server");
//        if(close(newSocketFd) < 0){
//            atExit("Error closing socket");
//        }
//        return;
//    }


//    pollFds.push_back({serverSockFd, POLLIN, 0});
}


bool TcpProxy::checkRecv(ConnectionContext& recvContext, pollfd& recvPollFd){
    bool toReturn = false;
    if(recvPollFd.revents & POLLIN){
        if((recvContext.bufLength = recv(recvContext.socketFd, recvContext.data, BUF_SIZE, 0)) < 0){
            perror("Error receiving data from client");
            return toReturn;
        }

        if(recvContext.bufLength == 0){
            throw SocketClosedException();
        }

        toReturn = true;
    }

    recvPollFd.revents = 0;
    return toReturn;
}

void TcpProxy::checkSend(ConnectionContext& from, ConnectionContext& to, pollfd& pollFd){
    if(pollFd.revents & POLLOUT){
        if(send(to.socketFd, from.data, from.bufLength, 0) < 0){
            atExit("Error sending data to client");
        }
    }
    pollFd.events = POLLIN;
    pollFd.revents = 0;
}

bool TcpProxy::checkPollFd(pollfd& pollFd, ConnectionContext& recv, ConnectionContext& send){
    bool isReceived = false;
    switch (pollFd.events){
        case POLLIN:
            isReceived = checkRecv(recv, pollFd);
            break;
        case POLLOUT:
            checkSend(send, recv, pollFd);
    }

    return isReceived;
}