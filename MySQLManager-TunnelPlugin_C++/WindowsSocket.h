#ifndef WINDOWSSOCKET_H
#define WINDOWSSOCKET_H

#include "BaseSocket.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <Ws2tcpip.h>
#include <inaddr.h>
#pragma comment (lib, "Ws2_32.lib")
#endif //!_WIN32

class WindowsSocket : public BaseSocket
{
public:
	WindowsSocket(Logger *logger);
	bool createSocket(int family, int socketType, int protocol, int port, int bufferLength, std::string ipAddress);
	bool createSocket(int family, int socketType, int protocol, int port, int bufferLength);
	bool bindAndStartListening();
	bool bindAndStartListening(int backlog);
	SOCKET *returnSocket();
	SOCKET *acceptClientAndReturnSocket(sockaddr_in *clientAddr);
	int sendToSocket(SOCKET *clientSocket, std::string dataToSend);
	std::string receiveDataOnSocket(SOCKET *socket);
	std::string getErrorStringFromErrorCode(int errorCode);
	void updateClassSocket(SOCKET *socket);
	void closeSocket();
    void closeSocket(SOCKET *socket);
private:
	SOCKET serverSocket;
	WSAData wsaData;
	int iResult;
	sockaddr_in serv_addr;
	struct addrinfo *result = NULL;
	struct addrinfo hints;
};
#endif //!WINDOWSSOCKET_H