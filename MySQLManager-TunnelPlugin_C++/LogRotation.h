#ifndef LOGROTATION_H
#define LOGROTATION_H

#include <iostream>
#include <mutex>
#include "INIParser.h"
#include "LogRotation.h"
#include <sstream>
#include <thread>
#ifndef STATUSMANAGER_H
#include "StatusManager.h"
#endif
#include <iostream>

using namespace std;

class LogRotation
{
public:
	LogRotation() {};
	//LogRotation(ofstream *logHandle);
	void rotateLogsIfRequired(ofstream *logHandle);
	bool loadLogRotateConfiguration(INIParser * const iniParser) const;
	void startLogRotation();
	~LogRotation();
	struct LogRotateConfiguration
	{
	public:
		static bool configurationLoaded;
		static long maxFileSizeInMB;
		static long maxArchiveSizeInMB;
		static string archiveDirectoryName;
		static int archiveSleepTimeInSeconds;
		static mutex logRotateMutex;
	};
private:
	void logRotationThread();
	thread logRotationMonitorThread;
	static bool logRotateThreadStarted;
    static bool logRotateShouldStop;
};
#endif