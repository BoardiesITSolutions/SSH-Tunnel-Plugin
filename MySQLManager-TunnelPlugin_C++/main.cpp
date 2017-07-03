#include <iostream>
#include "StaticSettings.h"
#include <sys/types.h>
#include <sstream>
#include <string.h>
#include "SocketListener.h"
#include <thread>
#include "TunnelManager.h"
#include "StatusManager.h"
#include "Logger.h"
#include "LogRotation.h"
#include "INIParser.h"
#include <signal.h>
#include <stdio.h>
#include <cstdlib>


using namespace std;

void signalHandler(int signal);
int ctrlCSignalCount = 0;

int main()
{

	Logger *logger = NULL;
	LogRotation *logRotation = NULL;

	

	try
	{
		StaticSettings staticSettings("tunnel.conf");
		staticSettings.readStaticSetting();

		logger = new Logger();

		INIParser iniParser("tunnel.conf");
		logRotation = new LogRotation();
		logRotation->loadLogRotateConfiguration(&iniParser);
		logRotation->startLogRotation();

		signal(SIGINT, signalHandler); //Ctrl +C

		StatusManager statusManager;
		statusManager.setApplicationStatus(StatusManager::ApplicationStatus::Running);

		//Start the tunnel monitor thread
		TunnelManager tunnelManager(logger);
		std::thread tunnelMonitorThread(&TunnelManager::tunnelMonitorThread, &tunnelManager);

		SocketListener socketListener(logger);
		socketListener.startSocketListener();

		logger->writeToLog("Successfully started. SSH tunneling can now be used");
		if (tunnelMonitorThread.joinable())
		{
			tunnelMonitorThread.join();
		}
	}
	catch (SocketException ex)
	{
		stringstream logstream;
		logstream << "Socket Exception: " << ex.what();
		if (logger != NULL)
		{
			logger->writeToLog(logstream.str());
		}
	}

	if (logger != NULL)
	{
		delete logger;
	}
	if (logRotation != NULL)
	{
		delete logRotation;
	}
	return EXIT_SUCCESS;
}

void signalHandler(int signal)
{
	switch (signal)
	{
	case SIGINT:

		cout << "Received SIGINT" << endl;
		ctrlCSignalCount++;
		if (ctrlCSignalCount == 1)
		{
			StatusManager statusManager;
			statusManager.setApplicationStatus(StatusManager::ApplicationStatus::Stopping);
			cout << "Received CTRL + C Signal. Stopping Threads. Press CTRL + C again to abort process" << endl;
		}
		else
		{
			cout << "Aborting process";
			raise(SIGABRT);
		}

		break;
	default:
		break;
	}
}