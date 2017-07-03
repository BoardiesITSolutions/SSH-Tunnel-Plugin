#pragma once
#ifndef SOCKETPROCESSOR_H

#include <thread>
#include "TunnelManager.h"
#include "StaticSettings.h"
#include "Logger.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#ifdef _WIN32
#include "WindowsSocket.h"
#else
#include "LinuxSocket.h"
#endif

class SocketProcessor
{
public:
	SocketProcessor(Logger *logger, void *socketManager);
	~SocketProcessor();
	void processSocketData(void *client);
	void processSocketDataThread(void *client);
private:
	std::thread socketProcessorThread;
	bool threadStarted = false;
	Logger *logger = NULL;
#ifdef _WIN32
	SOCKET *client;
	WindowsSocket *socketManager = NULL;
#else
	int *client;
	LinuxSocket *socketManager = NULL;
#endif
};

#endif //!SOCKETPROCESSOR_H