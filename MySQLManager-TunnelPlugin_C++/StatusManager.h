#pragma once

#ifndef STATUSMANAGER_H
#define STATUSMANAGER_H
#include <thread>
#include <map>
#include <string>
#include <iostream>
#include <map>
#include <vector>
class StatusManager
{
public:
	StatusManager();
	enum ApplicationStatus { Starting, Running, Stopping };
	void setApplicationStatus(ApplicationStatus applicationStatus);
	ApplicationStatus getApplicationStatus();
	
private:
	//static void signalHandler(int signal);
	static int ctrlCSignalCount;
	static StatusManager::ApplicationStatus appStatus;
	std::string convertEnumValueToString(ApplicationStatus appStatus);
};
#endif