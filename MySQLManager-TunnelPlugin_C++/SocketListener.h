#pragma once
#ifndef SOCKETLISTENER_H
#define SOCKETLISTENER_H

#include "BaseSocket.h"
#include <thread>
#include "StaticSettings.h"
#include "SocketProcessor.h"
#include <vector>
#ifdef _WIN32
#include "WindowsSocket.h"
#else
#include "LinuxSocket.h"
#endif //!_WIN32
#include <thread>
#include <chrono>
#include <mutex>
#include "Logger.h"
#include "StatusManager.h"

class SocketListener
{
public:
	SocketListener(Logger *logger);
	void startSocketListener();
	~SocketListener();
private:
	std::thread threadSocketListener;
	void socketListenerThread();
	bool threadStarted = false;
	vector<std::thread*> processingThreadList;
    static std::mutex socketListenerMutex;
	Logger *logger = NULL;
#ifdef _WIN32
	WindowsSocket socketManager = NULL;
#else
	LinuxSocket socketManager = NULL;
#endif //!_WIN32
};

#endif //!SOCKET_LISTENER_H