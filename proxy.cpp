#include <sys/socket.h>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <utility>
#include <arpa/inet.h>
#include "proxy.h"
#include "httpParser/httpParser.h"
#include "exceptions/socketClosedException.h"
#include "exceptions/unsupportedMethodException.h"
#include "models/Connection.h"

// один из сценариев тестирования прокси когда страница частично докачана, потом этот урл 
// запрашивает второй клиент, то если у него соединение с сервером всего 10 бит в секунду 
// он должен быстро получить ту часть данных, которая лежит в кеше, а потом получать  
// с дефолтной скоростью остальные данные

// вынести кеш в отдельный класс - со всеми операциями по обращению к данным

Proxy::Proxy(int portToListen, sockaddr_in serverAddress) : portToListen(portToListen), serverAddress(serverAddress){}

void Proxy::start(){
    int sockFd = initProxySocket();
    int pollState;

    pollFds.push_back({sockFd, POLLIN, 0});

    while (true){
        if((pollState = poll(pollFds.data(), pollFds.size(), -1)) < 0){
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

HttpMessage Proxy::parseHttpRequest(Connection& client) {
    int version;
    size_t methodLen, pathLen;
    const char *method, *path;
    struct phr_header headers[100];
    size_t num_headers = sizeof(headers) / sizeof(headers[0]);

    std::cout << "Client data:\n" << client.buffer->data() << std::endl;
    int reqSize = phr_parse_request(client.buffer->data(), client.buffer->size(), &method, &methodLen, &path, &pathLen,
                                    &version, headers, &num_headers, 0);

    if(reqSize == -1){
        errors[client.socketFd] = "HTTP/1.0 400\r\n\r\nBAD REQUEST\r\n";
    }

    char *url = new char[pathLen + 1];
    url[pathLen] = '\0';
    char *httpMethod = new char[methodLen + 1];
    httpMethod[methodLen] = '\0';

    return {(char *) memcpy(httpMethod, method, methodLen),
            (char *) memcpy(url, path, pathLen),
            version};


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

void Proxy::checkRequest(HttpMessage& request){

    if(strcmp(request.method, "GET") != 0 && strcmp(request.method, "HEAD") != 0){
        throw std::runtime_error("HTTP/1.0 501\r\n\r\nNOT IMPLEMENTED\r\n");
//        throw unsupportedMethodException();
    }

    if(request.version != 0){
        throw std::runtime_error("HTTP/1.1 505\r\n\r\nHTTP VERSION NOT SUPPORTED\r\n");
//        throw unsupportedMethodException();
    }

    // и остальные проверки
}

Connection Proxy::initServerConnection(Connection& clientConnection){
    int serverSockFd = socket(AF_INET, SOCK_STREAM, 0);

    if(serverSockFd < 0){
        throw std::runtime_error("Error connecting with server");
    }

    if(connect(serverSockFd, (struct sockaddr *)& serverAddress, sizeof(serverAddress)) == -1){
        throw std::runtime_error("Server DE4D!\n Can't connect");
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
    pollFds[clientConnection.pollFdsIndex].events = POLLOUT;
    HttpMessage httpMessage = parseHttpRequest(clientConnection);
    clientConnection.URl = httpMessage.path;
    checkRequest(httpMessage);

    return !cacheNodes.contains(httpMessage.path);
}

bool Proxy::checkForErrors(Connection& connection) {
    if (errors.find(connection.socketFd) == errors.end()) {
        return false;
    }

    char *errorMsg = errors[connection.socketFd];
    if (send(connection.socketFd, errorMsg, strlen(errorMsg), 0) < 0) {
        throw std::runtime_error("Error sending error message");
    }
    close(connection.socketFd);
    return true;
}

void Proxy::sendDataFromCache(Connection& connection){
    if(checkForErrors(connection)){
        removeClient(connection);
        return;
    }

    auto& cacheNode = cacheNodes.getCurrentData(connection.URl);
    int& offset = cacheOffsets[connection.socketFd];
    if(send(connection.socketFd, cacheNode.data() + offset, cacheNode.size() - offset, 0) < 0){
        removeClient(connection);
        perror("Error sending data to client from cache");
        return;
//        throw std::runtime_error("Error sending data to client from cache");
    }

    offset = cacheNode.size();
}

void Proxy::checkClientsConnections() {
    for(auto& clientConnection : clientConnections) {
        try {
            if (checkSend(clientConnection)) {
                sendDataFromCache(clientConnection);
            } else if (checkRecv(clientConnection)) {
                receiveData(clientConnection, false);
                if (isNewPathRequest(clientConnection))
                    serverConnections.push_back(initServerConnection(clientConnection));
            }
        }
        catch (std::runtime_error& error){
            std::cout << "CLIENT ERROR: \n" << error.what() << std::endl;
            errors.emplace(clientConnection.socketFd, error.what());
        }
        catch (SocketClosedException& exception){
            removeClient(clientConnection);
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

void Proxy::checkServerResponse(const char* URL, char* response, int responseLength, int socketFd){
    std::cout << response << std::endl;

    if(isCorrectResponseStatus(URL, response, responseLength)){
        cacheNodes.addData(URL, response, responseLength);
    }
    else{
        errors.emplace(socketFd, response);
    }
}

void Proxy::receiveData(Connection& connection, bool isServerConnection) {
    char buf[BUF_SIZE];
    int recvCount = 0;

    if ((recvCount = recv(connection.socketFd, buf, BUF_SIZE, 0)) < 0) {
        throw std::runtime_error("Error receiving data");
    }

    if (recvCount == 0) {
        throw SocketClosedException();
    }

    if(isServerConnection)
        checkServerResponse(connection.URl, buf, recvCount, connection.socketFd);
    else
        connection.buffer->insert(connection.buffer->end(), buf, buf + recvCount);
}

void Proxy::checkServerConnections(){
    for (auto &serverConnection : serverConnections) {
        try {
            if (checkRecv(serverConnection)) {
                receiveData(serverConnection, true);
            } else if (checkSend(serverConnection)) {
                sendRequestToServer(serverConnection);
                pollFds[serverConnection.pollFdsIndex].events = POLLIN;
            }

            pollFds[serverConnection.pollFdsIndex].revents = 0;
        }
        catch (std::runtime_error& error){
            std::cout << "SERVER ERROR: \n" << error.what() << std::endl;
            // TODO send error message to all clients connected to this server
//            errors.emplace(clientConnection.socketFd, error.what());
        }

        catch (SocketClosedException &exception) {
            removeClient(serverConnection);
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

template <typename T>
void remove(std::vector<T>& vector, T& toRemove){
    vector.erase(std::remove(vector.begin(), vector.end(), toRemove), vector.end());
}

void Proxy::removeClient(Connection &connection) {
    pollFds.erase(pollFds.begin() + connection.pollFdsIndex);
    errors.erase(connection.socketFd);
    cacheOffsets.erase(connection.socketFd);
    connection.buffer->clear();
    delete(connection.buffer);

    if(close(connection.socketFd) < 0 ){
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
    remove(clientConnections, connection);
}
