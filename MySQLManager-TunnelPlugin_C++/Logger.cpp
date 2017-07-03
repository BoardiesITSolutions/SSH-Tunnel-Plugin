/**
	This class writes debug information to a log file, and if the log file has grown to big (configured within the configuration file) 
	will be rotated, where the current log is closed and renamed with the date/time and on the next log a new file will be created
*/
#include "Logger.h"


using namespace std;

ofstream Logger::logHandle;

Logger::Logger()
{

}

/**
	Write debug information to the log. Providing the class name and method name can make it easier to debug as you know roughly where the debug line was written you know where the problem might be
	@param logLine The debug line that is to be writtenn to the log file
	@param className This is the name of the class file that is writing the debug line
	@param methodInfo The name of the method that is writing the debug line
*/
void Logger::writeToLog(string logLine, string className, string methodInfo)
{
	if (StaticSettings::AppSettings::logFile.empty())
	{
		INIParser iniParser("tunnel.conf");
		if (!iniParser.getKeyValueFromSection("general", "logFile", &StaticSettings::AppSettings::logFile))
		{
			cout << "Can't find log file in config. Defaulting to tunnel.conf" << endl;
			StaticSettings::AppSettings::logFile = "tunnel.conf";
		}
	}

	//Opens the log file in append mode - creates if it doesn't exist
	logHandle.open(StaticSettings::AppSettings::logFile.c_str(), fstream::app);
	LogRotation::LogRotateConfiguration::logRotateMutex.lock();
	time_t t = time(0);
	struct tm * now = localtime(&t);
	stringstream logstream;

	char date[21];
	strftime(date, 21, "%d/%m/%Y %H:%M:%S", now);

	logstream << date << ":\t";

	if (!className.empty() && !methodInfo.empty())
	{
		logstream << className << "/" << methodInfo << ":\t";
	}

	logstream << logLine;

	logHandle << logstream.str() << endl;
	logHandle.flush();
	cout << logstream.str() << endl;
	LogRotation logRotation;
	logRotation.rotateLogsIfRequired(&logHandle);
	logHandle.close();
	LogRotation::LogRotateConfiguration::logRotateMutex.unlock();
}

/**
	Write a message to the log file, but without the class name and method name. This should only be used for general messages, that don't indiciate a specific issue
	where debugging something is not going to be required, for example, we use this for stating that the application is ready for SSH tunnelling
*/
void Logger::writeToLog(string logLine)
{
	this->writeToLog(logLine, "", "");
}

Logger::~Logger()
{
	if (logHandle.is_open())
	{
		logHandle.close();
	}
}