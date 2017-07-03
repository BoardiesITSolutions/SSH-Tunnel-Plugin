#ifndef LINUXSOCKET_H
#define LINUXSOCKET_H

#ifndef _WIN32
#include "BaseSocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
class LinuxSocket : public BaseSocket
{
    public:
        LinuxSocket(Logger *logger);
        bool createSocket(int family, int socketType, int protocol, int port, int bufferLength);
        bool createSocket(int family, int socketType, int protocol, int port, int bufferLength, std::string ipAddress); 
        bool bindAndStartListening();
        int returnSocket();
        int acceptClientAndReturnSocket(sockaddr_in *clientAddr);
        int sendToSocket(int *socket, std::string dataToSend);
        std::string receiveDataOnSocket(int * socket);
        void closeSocket();
        void closeSocket(int *socket);
    private:
        int serverSocket;
        sockaddr_storage *serv_addr = NULL;
        sockaddr_storage *cli_addr;
        std::string ipAddress;
        size_t servAddressSize;
};

#endif //!LINUXSOCKET_H
#endif //!_WIN32