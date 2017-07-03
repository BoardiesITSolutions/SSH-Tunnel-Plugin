#ifndef BASESOCKET_H
#define BASESOCKET_H

#include <iostream>
#include "Logger.h"
#include <string>
#include "SocketException.h"

class BaseSocket
{
public:
	BaseSocket(Logger *logger);
	virtual bool createsocket(int port, int bufferLength);
	virtual bool bindAndStartListening(int backlog) { return true; };
	virtual bool bindAndStartListening() { return true; };
	virtual int sendToSocket(void* clientSocket, std::string dataToSend) { return 0; };
	virtual std::string getErrorStringFromErrorCode(int errorCode) { return string(); };
	virtual std::string receiveDataOnSocket(void* socket) { return string(); };
    virtual void closeSocket() {};
    virtual void closeSocket(void* socket) {};
	virtual void updateClassSocket(void *socket) {};
protected:
	Logger *logger = NULL;
	int bufferLength;
	char *buffer = NULL;
	int socketPort;
};

#endif //!BASESOCKET_H#pragma once
