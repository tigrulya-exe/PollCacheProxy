#include <sys/socket.h>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <arpa/inet.h>
#include "proxy.h"
#include "exceptions/socketClosedException.h"
#include "models/Connection.h"
#include "models/HttpRequest.h"

// один из сценариев тестирования прокси когда страница частично докачана, потом этот урл 
// запрашивает второй клиент, то если у него соединение с сервером всего 10 бит в секунду 
// он должен быстро получить ту часть данных, которая лежит в кеше, а потом получать  
// с дефолтной скоростью остальные данные

// вынести кеш в отдельный класс - со всеми операциями по обращению к данным

Proxy::Proxy(int portToListen, sockaddr_in serverAddress) : portToListen(portToListen), serverAddress(serverAddress){}

void Proxy::start(){
    int sockFd = initProxySocket();

    pollFds.push_back({sockFd, POLLIN, 0});

    while (true){
        if(poll(pollFds.data(), pollFds.size(), -1) < 0){
            perror("Poll");
            exit(EXIT_FAILURE);
        }

        if(pollFds[ACCEPT_INDEX].revents & POLLIN){
            addNewConnection(sockFd);
        }

        checkClientsConnections();
        checkServerConnections();
    }
}

HttpRequest Proxy::parseHttpRequest(Connection& client) {
    HttpRequest httpRequest;

    httpRequest.headersCount = sizeof(httpRequest.headers) / sizeof(httpRequest.headers[0]);
    std::cout << "Client data:\n" << client.buffer->data() << std::endl;
    int reqSize = phr_parse_request(client.buffer->data(), client.buffer->size(), &httpRequest.method, &httpRequest.methodLen,
                                    &httpRequest.path, &httpRequest.pathLen, &httpRequest.version, httpRequest.headers,
                                    &httpRequest.headersCount, 0);

    if(reqSize == -1){
        throw std::runtime_error("HTTP/1.0 400\r\n\r\nBAD REQUEST\r\n");
    }

    return httpRequest;


//    checkUrl(url, client);
    // printf("request is %d bytes long\n", reqSize);
    // printf("method is %.*s\n", (int)methodLen, method);
    // printf("path is %.*s\n", (int)pathLen, path);
    // printf("HTTP version is 1.%d\n", version);
    // printf("headers:\n");
    // for (int i = 0; i != num_headers; ++i) {
    //     printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name,
    //     (int)headers[i].value_len, headers[i].value);
    // }
}

void Proxy::checkRequest(HttpRequest& request){

    if(strstr(request.method, "GET") == nullptr && strstr(request.method, "HEAD") == nullptr){
        throw std::runtime_error("HTTP/1.0 501\r\n\r\nNOT IMPLEMENTED\r\n");
    }

//    if(request.version != 0){
//        throw std::runtime_error("HTTP/1.1 505\r\n\r\nHTTP VERSION NOT SUPPORTED\r\n");
//    }

    // и остальные проверки
}


Connection Proxy::initServerConnection(Connection& clientConnection){
    int serverSockFd = socket(AF_INET, SOCK_STREAM, 0);

    if(serverSockFd < 0){
        throw std::runtime_error("HTTP/1.0 500\r\n\r\nERROR CONNECTING WITH SERVER\r\n");
    }

    if(connect(serverSockFd, (struct sockaddr *)& serverAddress, sizeof(serverAddress)) == -1){
        throw std::runtime_error("HTTP/1.0 500\r\n\r\nERROR CONNECTING WITH SERVER\r\n");
    }

    // будем ждать готовности сервера к записи
    pollFds.push_back({serverSockFd, POLLOUT, 0});
    Connection serverConnectionOut =  Connection( serverSockFd, pollFds.size() - 1);
    cacheNodes.setNodeReady(clientConnection.URl, false);
    serverConnectionOut.URl = clientConnection.URl;
    serverConnectionOut.buffer = clientConnection.buffer;

    return serverConnectionOut;
}

bool Proxy::isNewPathRequest(Connection& clientConnection) {
    // на этом этапе надо как нить чекнуть что у клиента есть весь хттп фрейм целиком
    // TODO
    pollFds[clientConnection.pollFdsIndex].revents = 0;

    HttpRequest httpRequest = parseHttpRequest(clientConnection);

    clientConnection.URl = new char[httpRequest.pathLen];
    clientConnection.URl = (char* ) memcpy((void *) clientConnection.URl, httpRequest.path, httpRequest.pathLen);
    checkRequest(httpRequest);

    return !cacheNodes.contains(clientConnection.URl);
}

bool Proxy::checkForErrors(Connection& connection) {
    if (errors.find(connection.socketFd) == errors.end()) {
        return false;
    }

    char *errorMsg = errors[connection.socketFd];
    if (send(connection.socketFd, errorMsg, strlen(errorMsg), 0) < 0) {
        throw std::runtime_error("Error sending error message");
    }
    return true;
}

void Proxy::sendDataFromCache(Connection& connection){
    if(checkForErrors(connection)){
        removeClientConnection(connection);
        return;
    }

    auto& cacheNode = cacheNodes.getCurrentData(connection.URl);
    int& offset = cacheOffsets[connection.socketFd];
    if(send(connection.socketFd, cacheNode.data() + offset, cacheNode.size() - offset, 0) < 0){
        removeClientConnection(connection);
        perror("Error sending data to client from cache");
        return;
//        throw std::runtime_error("Error sending data to client from cache");
    }


    std::cout << "I GOT DATA (TOTAL SIZE - " << cacheNode.size() - offset <<  " BYTES) FROM CACHE!" << std::endl;
    offset = cacheNode.size();


    pollFds[connection.pollFdsIndex].events = POLLIN;
}

void Proxy::handleClientDataReceive(Connection& clientConnection){
    receiveDataFromClient(clientConnection);
    if (isNewPathRequest(clientConnection))
        serverConnections.push_back(initServerConnection(clientConnection));
    else
        pollFds[clientConnection.pollFdsIndex].events = POLLOUT;
}

void Proxy::checkClientsConnections() {
    for(auto& clientConnection : clientConnections) {
        try {
            if (checkSend(clientConnection)) {
                sendDataFromCache(clientConnection);
                if(cacheNodes.cacheNodeReady(clientConnection.URl)){
                    removeClientConnection(clientConnection);
                }
            } else if (checkRecv(clientConnection)) {
                handleClientDataReceive(clientConnection);
            }
        }
        catch (std::runtime_error& error){
            std::cout << "CLIENT ERROR: \n" << error.what() << std::endl;
            errors.emplace(clientConnection.socketFd, (char*)error.what());
        }
        catch (SocketClosedException& exception){
            removeClientConnection(clientConnection);
        }
    }
}


bool Proxy::checkSend(Connection& connection){
    return pollFds[connection.pollFdsIndex].revents & POLLOUT;
}

bool Proxy::checkRecv(Connection& connection){
    return pollFds[connection.pollFdsIndex].revents & POLLIN;
}

void Proxy::sendRequestToServer(Connection& serverConnection){
    if(send(serverConnection.socketFd, serverConnection.buffer->data(), serverConnection.buffer->size(), 0) < 0){
        throw std::runtime_error("Error sending data to server");
    }

    pollFds[serverConnection.pollFdsIndex].events = POLLIN;
}

bool Proxy::isCorrectResponseStatus(const char *URL, char *response, int responseLength) {
    size_t bodyLen, pathLen;
    const char *body, *path;
    int version, status = 0;
    struct phr_header headers[100];
    size_t num_headers = sizeof(headers) / sizeof(headers[0]);

    phr_parse_response(response, responseLength, &version, &status, &body, &bodyLen, headers, &num_headers, 0);

    return status == 200;
}

void Proxy::checkServerResponse(const char* URL, char* response, int responseLength){
    std::cout << response << std::endl;

    if(!isCorrectResponseStatus(URL, response, responseLength)) {
        throw std::runtime_error(response);
    }

    cacheNodes.addData(URL, response, responseLength);
}

void Proxy::prepareClientsToWrite(const char* URL){
    for(auto& client: clientConnections){
        if(!strcmp(client.URl, URL)){
            pollFds[client.pollFdsIndex].events = POLLOUT;
        }
    }
}

void Proxy::receiveDataFromServer(Connection &connection) {
    static char buf[BUF_SIZE];
    int recvCount = receiveData(connection, buf);

    prepareClientsToWrite(connection.URl);
    if(!recvCount){
        cacheNodes.setNodeReady(connection.URl, true);
        throw SocketClosedException();
    }

    checkServerResponse(connection.URl, buf, recvCount);
}

void Proxy::receiveDataFromClient(Connection &connection) {
    static char buf[BUF_SIZE];
    int recvCount = receiveData(connection, buf);

    if(!recvCount){
        throw SocketClosedException();
    }

    connection.buffer->insert(connection.buffer->end(), buf, buf + recvCount);
}

int Proxy::receiveData(Connection& connection, char* buf) {
    int recvCount = 0;

    if ((recvCount = recv(connection.socketFd, buf, BUF_SIZE, 0)) < 0) {
        throw std::runtime_error("Error receiving data");
    }

    return recvCount;
}

void Proxy::notifyClientsAboutError(const char* URL,const char* error){
    for(auto& client: clientConnections){
        if(!strcmp(client.URl, URL)){
            errors.emplace(client.socketFd, (char*)error);
            pollFds[client.pollFdsIndex].events = POLLOUT;
        }
    }
}

void Proxy::checkServerConnections(){
    for (auto &serverConnection : serverConnections) {
        try {
            if (checkRecv(serverConnection)) {
                receiveDataFromServer(serverConnection);
            } else if (checkSend(serverConnection)) {
                sendRequestToServer(serverConnection);
            }

            pollFds[serverConnection.pollFdsIndex].revents = 0;
        }

        catch (std::runtime_error& error){
            std::cout << "SERVER ERROR: \n" << error.what() << std::endl;
            notifyClientsAboutError(serverConnection.URl, error.what());
            removeServerConnection(serverConnection);
        }

        catch (SocketClosedException &exception) {
            removeServerConnection(serverConnection);
        }
    }
}

void Proxy::initAddress(sockaddr_in* addr, int port){
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(port);
}

int Proxy::initProxySocket(){
    sockaddr_in address;
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

void Proxy::addNewConnection(int sockFd){
    int newSocketFd = accept(sockFd, nullptr, nullptr);

    if(newSocketFd < 0){
        perror("Error accepting connection");
        return;
    }

    pollFds.push_back({newSocketFd, POLLIN, 0});
    clientConnections.emplace_back(newSocketFd, pollFds.size() - 1);
    clientConnections.rbegin()->initBuffer();
}

void remove(std::vector<Connection>& vector, Connection& toRemove){
    vector.erase(std::remove_if(vector.begin(), vector.end(), [&toRemove](const Connection& connection) {
        return connection.socketFd == toRemove.socketFd;
    }), vector.end());
}

void updatePollFdIndexes(std::vector<Connection>& connections, int deletedPollFdIndex){
    for(auto& connection : connections){
        if(connection.pollFdsIndex > deletedPollFdIndex){
            --connection.pollFdsIndex;
        }
    }
}

void Proxy::removeConnection(Connection& connection){
    pollFds.erase(pollFds.begin() + connection.pollFdsIndex);

    if(close(connection.socketFd) < 0 ){
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
    updatePollFdIndexes(serverConnections, connection.pollFdsIndex);
    updatePollFdIndexes(clientConnections, connection.pollFdsIndex);
}

void Proxy::removeServerConnection(Connection& connection){
    removeConnection(connection);
    remove(serverConnections, connection);
}


void Proxy::removeClientConnection(Connection &connection) {
    errors.erase(connection.socketFd);
    cacheOffsets.erase(connection.socketFd);
    connection.buffer->clear();
    delete(connection.buffer);

    removeConnection(connection);
    remove(clientConnections, connection);
}