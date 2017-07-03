/**
	Controls the status of the application, used for the various while loops to check if it has been shutdown
*/

#include "StatusManager.h"

#include <iostream>
#include <thread>
#include <chrono>


using namespace std;

StatusManager::ApplicationStatus StatusManager::appStatus = StatusManager::ApplicationStatus::Starting;

StatusManager::StatusManager()
{
}

/**
	Sets the application status, this should only be changed once the application has fully loaded, or when it is being shutdown. It can also be used if a critical fault
	occurs where the application shouldn't continue, you can set the application status to stopping, so any loops finish and the application will gracefully shutdown
	@param appStatus The status of that application should be put into it, e.g. Starting, Running, Stopping
*/
void StatusManager::setApplicationStatus(ApplicationStatus appStatus)
{
	this->appStatus = appStatus;
}

/**
	Get the current application status, if you are creating or modifying any loops, always checking that this is status is NOT stopping so that the while runs while starting, and running
*/
StatusManager::ApplicationStatus StatusManager::getApplicationStatus()
{
	return StatusManager::appStatus;
}

/**	
	If  you need to print the application status to the log file use this method to get the actual status string instead of the enum value
	@param appStatus, the status of the application where you want the string returned, StatusManaget::getApplicationStatus() should be used here
	@return string The string that corresponds with the enum value
*/
string StatusManager::convertEnumValueToString(ApplicationStatus appStatus)
{
	switch (appStatus)
	{
	case Starting:
		return "Starting";
	case Running:
		return "Running";
	case Stopping:
		return "Stopping";
	default:
		return "Unknown Status";
	}
}

