#include <sys/socket.h>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <algorithm>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <csignal>
#include <cstring>
#include "Proxy.h"
#include "exceptions/SocketClosedException.h"
#include "models/Connection.h"
#include "models/HttpRequest.h"
#include "exceptions/ProxyException.h"

// wget -r
// запустив два таких вгета сравнить надо деревья, деревья архивировать в тар и посчитать мд5 суммы и сравнить
// проксей скачать исо и сравнить ша суммы
//
//

/* сценарий - сервер с картинками - много маленьких картинок + кеширование маленьких элементов
 * потом обновляем страницу и чекаем что картинки взяты из кеша
 *
 * в курле ставим медленную скорость на одном клиенте качаем
 * на другом
 *
 * + логирование - чтобы знать что данные взяты из кеша
 *
 * можно юзать симпл хттп сервер чтобы чекать что не ходили к серваку
 * */


// один из сценариев тестирования прокси когда страница частично докачана, потом этот урл 
// запрашивает второй клиент, то если у него соединение с сервером всего 10 бит в секунду 
// он должен быстро получить ту часть данных, которая лежит в кеше, а потом получать  
// с дефолтной скоростью остальные данные

// вынести кеш в отдельный класс - со всеми операциями по обращению к данным
//CC -lrt -lxnet -lnsl  *.cpp -std=c++11 -m64 -lgcc_s -lc -o proxy

bool isInterrupted = false;

void interrupt(int sig){
    isInterrupted = true;
}

Proxy::Proxy(int portToListen) : portToListen(portToListen){}

void Proxy::stop(){
    std::cout << "Stopping server..." << std::endl;

    for(auto& pollFd : pollFds){
        if(close(pollFd.fd) < 0){
            perror("Error closing socket");
        }
    }

    for(auto& clientConnection : clientConnections){
        delete clientConnection.buffer;
    }
}

void Proxy::setSignalHandlers(){
    struct sigaction sigAction;

    sigAction.sa_handler = interrupt;
    sigemptyset (&sigAction.sa_mask);
    sigAction.sa_flags = 0;

    sigaction (SIGINT, &sigAction, nullptr);
    sigaction (SIGQUIT, &sigAction, nullptr);
    sigaction (SIGTERM, &sigAction, nullptr);

    sigAction.sa_handler = SIG_IGN;
    sigaction (SIGPIPE, &sigAction, nullptr);
}

void Proxy::start(){
    setSignalHandlers();
    int sockFd = initProxySocket();

    pollFds.push_back({sockFd, POLLIN, 0});

    while (true){
        if(isInterrupted) {
            stop();
            throw ProxyException("Interrupted");
        }

        if(poll(pollFds.data(), pollFds.size(), -1) < 0){
            stop();
            throw ProxyException(std::string("Poll error: ") + strerror(errno));
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

    const char* path;
    const char* method;
    int version;
    size_t methodLen, pathLen;

    httpRequest.headersCount = sizeof(httpRequest.headers) / sizeof(httpRequest.headers[0]);
    int reqSize = phr_parse_request(client.buffer->data(), client.buffer->size(), &method, &methodLen,
                                    &path, &pathLen, &version, httpRequest.headers,
                                    &httpRequest.headersCount, 0);

    if(reqSize == -1){
        throw ProxyException(errors::BAD_REQUEST);
    }

    std::string onlyPath = path;
    onlyPath.erase(onlyPath.begin() + pathLen, onlyPath.end());
    httpRequest.path = onlyPath;

    std::string onlyMethod = method;
    onlyMethod.erase(onlyMethod.begin() + methodLen, onlyMethod.end());
    httpRequest.method = onlyMethod;

    for(int i = 0; i < httpRequest.headersCount; ++i){
        std::string headerName = httpRequest.headers[i].name;
        headerName.erase(headerName.begin() + httpRequest.headers[i].name_len, headerName.end());

        if(headerName == "Host") {
            std::string headerValue = httpRequest.headers[i].value;
            headerValue.erase(headerValue.begin() + httpRequest.headers[i].value_len, headerValue.end());
            httpRequest.host = headerValue;
        }
    }

    return httpRequest;
}

void Proxy::checkRequest(HttpRequest& request){

    if(request.method == "GET" && request.method == "HEAD"){
        throw ProxyException(errors::NOT_IMPLEMENTED);
    }

//    if(request.version != 0){
//        throw std::runtime_error("HTTP/1.1 505\r\n\r\nHTTP VERSION NOT SUPPORTED\r\n");
//    }

}

sockaddr_in Proxy::getServerAddress(const char *host) {
    sockaddr_in serverAddr;

    addrinfo addrInfo = {0};
    addrInfo.ai_flags = 0;
    addrInfo.ai_family = AF_INET;
    addrInfo.ai_socktype = SOCK_STREAM;
    addrInfo.ai_protocol = IPPROTO_TCP;

    addrinfo* hostAddr = nullptr;
    getaddrinfo(host, nullptr, &addrInfo, &hostAddr);

    if (!hostAddr) {
        std::cout << "Can't resolve host!" << std::endl;
        throw ProxyException(errors::BAD_REQUEST);
    }

    serverAddr = *(sockaddr_in*) (hostAddr->ai_addr);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80);

    return serverAddr;
}

Connection Proxy::initServerConnection(Connection& clientConnection, HttpRequest& request){
    int serverSockFd = socket(AF_INET, SOCK_STREAM, 0);

    if(serverSockFd < 0){
        throw ProxyException(errors::SERVER_CONNECT);
    }

    sockaddr_in addressToConnect = getServerAddress(request.host.c_str());
    std::cout << "HOST: " << request.host << " : URL: " << clientConnection.URl << std::endl;

    fcntl(serverSockFd, F_SETFL, fcntl(serverSockFd, F_GETFL, 0) | O_NONBLOCK);

    if(connect(serverSockFd, (struct sockaddr *)& addressToConnect, sizeof(addressToConnect)) == -1 && errno != EINPROGRESS){
        throw ProxyException(errors::SERVER_CONNECT);
    }

    fcntl(serverSockFd, F_SETFL,  fcntl(serverSockFd, F_GETFL, 0) & ~O_NONBLOCK);

    // будем ждать готовности сервера к записи
    pollFds.push_back({serverSockFd, POLLOUT, 0});
    Connection serverConnectionOut =  Connection( serverSockFd, pollFds.size() - 1);
    cacheNodes.setNodeReady(clientConnection.URl, false);

    serverConnectionOut.URl = clientConnection.URl;
    serverConnectionOut.buffer = clientConnection.buffer;

    return serverConnectionOut;
}


ConnectionIter eraseConnection(std::vector<Connection>& vector, Connection& toRemove){
    return vector.erase(std::remove_if(vector.begin(), vector.end(), [&toRemove](const Connection& connection) {
        return connection.socketFd == toRemove.socketFd;
    }), vector.end());
}

bool Proxy::checkForErrors(Connection& connection) {
    if (errors.find(connection.socketFd) == errors.end()) {
        return false;
    }

    std::cout << "ERROR MESSAGE WAS SENT: " << connection.URl << std::endl;

    std::string& errorMsg = errors[connection.socketFd];
    if (send(connection.socketFd, errorMsg.c_str(), errorMsg.size(), 0) < 0 && errno != EWOULDBLOCK ) {
        std::cout << "Error sending error message" << std::endl;
    }
    return true;
}

bool Proxy::sendDataFromCache(Connection& connection, bool cacheNodeReady){
    if(checkForErrors(connection)){
        throw SocketClosedException();
    }

    auto& cacheNode = cacheNodes.getCurrentData(connection.URl);
    int& offset = cacheOffsets[connection.socketFd];

    int sendCount;
    if((sendCount = send(connection.socketFd, cacheNode.data() + offset, cacheNode.size() - offset, 0)) < 0 && errno != EWOULDBLOCK){
        perror("Error sending data to client from cache");
        throw ProxyException(errors::CACHE_SEND_ERROR);
    }

    if(sendCount)
        std::cout << connection.socketFd << " : GOT DATA ( " << sendCount <<  " BYTES) FROM CACHE:" << connection.URl << std::endl;

    offset += sendCount;

    if(!cacheNodeReady || cacheNode.size() == offset)
        pollFds[connection.pollFdsIndex].events = -1;

    return cacheNodeReady && !(cacheNode.size() - offset);
}

void Proxy::handleClientDataReceive(Connection& clientConnection){
    if(!receiveDataFromClient(clientConnection))
        return;

    HttpRequest httpRequest = parseHttpRequest(clientConnection);
    checkRequest(httpRequest);

    clientConnection.URl = httpRequest.path;
    pollFds[clientConnection.pollFdsIndex].revents = 0;

    if (!cacheNodes.contains(clientConnection.URl)){
        std::cout << "NEW PATH: " << clientConnection.URl << std::endl;
        serverConnections.push_back(initServerConnection(clientConnection, httpRequest));
    }
    else{
        pollFds[clientConnection.pollFdsIndex].events = POLLOUT;
    }
}

void Proxy::checkIfError(Connection& connection) {
    pollfd &pollFd = pollFds[connection.pollFdsIndex];
    if (pollFd.revents & POLLHUP || pollFd.revents & POLLERR || pollFd.revents & POLLNVAL) {
        throw SocketClosedException();
    }
}

bool Proxy::allDataHasBeenSent(Connection &clientConnection){
    bool cacheNodeReady = cacheNodes.cacheNodeReady(clientConnection.URl);
    bool allDataHasBeenSent = sendDataFromCache(clientConnection, cacheNodeReady);
    return  cacheNodeReady && allDataHasBeenSent;
}

void Proxy::checkClientsConnections() {
    for(auto iterator = clientConnections.begin(); iterator != clientConnections.end(); ) {
        Connection& clientConnection = *iterator;
        try {
            checkIfError(clientConnection);
            if (checkSend(clientConnection)) {
                if (allDataHasBeenSent(clientConnection)) {
                    std::cout << clientConnection.socketFd << " : i've downloaded all what i need" << std::endl;
                    iterator = removeClientConnection(clientConnection);
                    continue;
                }
            } else if (checkRecv(clientConnection)) {
                handleClientDataReceive(clientConnection);
            }
            ++iterator;
        }

        catch (ProxyException& error){
            errors.emplace(clientConnection.socketFd, (char*)error.what());
            checkForErrors(clientConnection);
            iterator = removeClientConnection(clientConnection);
        }
        catch (SocketClosedException& exception){
            std::cout << "Socket closed: " << clientConnection.URl << std::endl;
            iterator = removeClientConnection(clientConnection);
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

    if(send(serverConnection.socketFd, serverConnection.buffer->data(), serverConnection.buffer->size(), 0) < 0 && errno != EWOULDBLOCK){
        throw ProxyException(errors::SERVER_CONNECT);
    }

    pollFds[serverConnection.pollFdsIndex].events = POLLIN;
}

bool Proxy::isCorrectResponseStatus(char *response, int responseLength) {
    size_t bodyLen;
    const char *body;
    int version, status = 0;
    struct phr_header headers[100];
    size_t num_headers = sizeof(headers) / sizeof(headers[0]);

    phr_parse_response(response, responseLength, &version, &status, &body, &bodyLen, headers, &num_headers, 0);

    return status == 200;
}


void Proxy::prepareClientsToWrite(std::string& URL){
    for(auto& client: clientConnections){
        if(client.URl == URL){
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

    if(!cacheNodes.contains(connection.URl) && !isCorrectResponseStatus(buf, recvCount))
        throw ProxyException(buf);

    cacheNodes.addData(connection.URl, buf, recvCount);
}

bool Proxy::receiveDataFromClient(Connection &connection) {
    static char buf[BUF_SIZE];
    int recvCount = receiveData(connection, buf);

    if(!recvCount){
        throw SocketClosedException();
    }

    connection.buffer->insert(connection.buffer->end(), buf, buf + recvCount);
    return (buf[recvCount - 1] == '\n' && buf[recvCount - 2] == '\r' && buf[recvCount - 3] == '\n' && buf[recvCount - 4] == '\r');
}

int Proxy::receiveData(Connection& connection, char* buf) {
    int recvCount = 0;

    if ((recvCount = recv(connection.socketFd, buf, BUF_SIZE, 0)) < 0) {
        throw ProxyException(errors::INTERNAL_ERROR);
    }

    return recvCount;
}

void Proxy::notifyClientsAboutError(std::string& URL,const char* error){
    for(auto& client: clientConnections){
        if(client.URl == URL){
            errors[client.socketFd] = (char*)error;
            pollFds[client.pollFdsIndex].events = POLLOUT;
        }
    }
}

void Proxy::checkServerConnections(){
    for(auto iterator = serverConnections.begin(); iterator != serverConnections.end();) {
        Connection& serverConnection = *iterator;
        try {
            checkIfError(serverConnection);
            if (checkRecv(serverConnection)) {
                receiveDataFromServer(serverConnection);
            } else if (checkSend(serverConnection)) {
                sendRequestToServer(serverConnection);
            }
            ++iterator;
            pollFds[serverConnection.pollFdsIndex].revents = 0;
        }

        catch (ProxyException& error){
            notifyClientsAboutError(serverConnection.URl, error.what());
            iterator = removeServerConnection(serverConnection);
        }

        catch (SocketClosedException &exception) {
            iterator = removeServerConnection(serverConnection);
        }
    }
}

void Proxy::initAddress(sockaddr_in* addr, int port){
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(port);
}

int Proxy::initProxySocket(){
    int reuse = 1;
    sockaddr_in address;
    initAddress(&address, portToListen);


    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

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
    fcntl(newSocketFd, F_SETFL, fcntl(newSocketFd, F_GETFL, 0) | O_NONBLOCK);

    pollFds.push_back({newSocketFd, POLLIN, 0});
    clientConnections.emplace_back(newSocketFd, pollFds.size() - 1);
    cacheOffsets[newSocketFd] = 0;
    clientConnections.rbegin()->initBuffer();
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
    updatePollFdIndexes(serverConnections, connection.pollFdsIndex);
    updatePollFdIndexes(clientConnections, connection.pollFdsIndex);

    if(close(connection.socketFd) < 0 ){
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}

ConnectionIter Proxy::removeServerConnection(Connection& connection){
    removeConnection(connection);
    return eraseConnection(serverConnections, connection);
}

ConnectionIter Proxy::removeClientConnection(Connection &connection) {
    errors.erase(connection.socketFd);
    cacheOffsets.erase(connection.socketFd);
    delete(connection.buffer);

    removeConnection(connection);
    return eraseConnection(clientConnections, connection);
}