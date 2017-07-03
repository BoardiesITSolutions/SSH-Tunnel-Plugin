/**
	This manages the rotation of the log file. This avoids too much disk space being used. This is configurable in the config file, ensure that the maxArchiveDirectorySize isn't too large
	that it will use too much of your disk. Once the log file reaches the max size set in the config file (defaults to 50MB) the log file is closed and renamed with the date/time of when
	the rotation occurred, then on the next log line being written a new file will be created.
	If the archive grows too big (based on the maxArchiveSizeInMB in the configuration file) then the oldest file in the archive will be deleted
*/

#include "LogRotation.h"
#include "INIParser.h"
#include "StaticSettings.h"
#include <ctime>
#include <stdio.h>
#include <stdarg.h>
#include <cstdio>
#include "HelperMethods.h"
#ifdef _WIN32
#include <Windows.h>
#endif
#include <signal.h>
#include <chrono>
#include <boost/filesystem.hpp>
#include <sys/stat.h>


using namespace std;

//Log Rotate Config
bool LogRotation::LogRotateConfiguration::configurationLoaded = false;
long LogRotation::LogRotateConfiguration::maxFileSizeInMB = 0;
long LogRotation::LogRotateConfiguration::maxArchiveSizeInMB = 0;
string LogRotation::LogRotateConfiguration::archiveDirectoryName = "";
int LogRotation::LogRotateConfiguration::archiveSleepTimeInSeconds = 0;
mutex LogRotation::LogRotateConfiguration::logRotateMutex;
bool LogRotation::logRotateThreadStarted = false;
bool LogRotation::logRotateShouldStop = false;

typedef LogRotation::LogRotateConfiguration config;

/*LogRotation::LogRotation(ofstream *logHandle)
{
        this->logHandle = logHandle;
}*/

/**
	Load the configuration settings for the log rotation. This HAS to be called, before starting the thread
*/
bool LogRotation::loadLogRotateConfiguration(INIParser * const iniParser) const {
    if (!iniParser->getKeyValueFromSection("log_rotate", "maxFileSizeInMB", &config::maxFileSizeInMB)) {
        cout << "Unable to read 'log_rotate' key 'maxFileSizeInBytes'. Cannot continue" << endl;
        return false;
    }
    if (!iniParser->getKeyValueFromSection("log_rotate", "maxArchiveSizeInMB", &config::maxArchiveSizeInMB)) {
        cout << "Unable to read 'log_rotate' key 'maxArchiveSizeInBytes'. Cannot continue" << endl;
        return false;
    }
    if (!iniParser->getKeyValueFromSection("log_rotate", "archiveDirectoryName", &config::archiveDirectoryName)) {
        cout << "Unable to read 'log_rotate' key 'archiveDirectoryName'. Defaulting to 'archive'" << endl;
        config::archiveDirectoryName = "archive";
    }
    if (!iniParser->getKeyValueFromSection("log_rotate", "archiveSleepTimeInSeconds", &config::archiveSleepTimeInSeconds)) {
        cout << "Unable to read 'log_rotate' key 'archiveSleepTimeInSeconds'. Defaulting to 60 seconds" << endl;
        config::archiveSleepTimeInSeconds = 60;
    }

    HelperMethods helperMethods;
    if (!helperMethods.doesDirectoryExist(LogRotateConfiguration::archiveDirectoryName)) {
#ifdef _WIN32
        CreateDirectory(LogRotateConfiguration::archiveDirectoryName.c_str(), NULL);
#else
        mkdir(LogRotateConfiguration::archiveDirectoryName.c_str(), 755);
#endif
    }

    LogRotateConfiguration::configurationLoaded = true;
    return true;
}

/**
	Close the log and move it to the archive with the date/time string of when the rotation occurred
	If the rotation fails, to ensure we don't end up writing to the same log file, potentially filling the disk, we'll raise a SIGABRT and stop it running
	@param logHandle The log handle of the file that is to be rotated
*/
void LogRotation::rotateLogsIfRequired(ofstream *logHandle) {
    //We need to close the file handle, and open in a different mode to get the file size
    logHandle->close();

    ifstream fileStream;
    fileStream.open(StaticSettings::AppSettings::logFile, ifstream::ate | ifstream::binary);
    long fileSize = fileStream.tellg();
    fileStream.close();

    //Convert the size to mb
    fileSize = (float) fileSize / (float) 1024 / (float) 1024;

    if (fileSize >= LogRotateConfiguration::maxFileSizeInMB) {
        //The log file is greater than the max size, so archive the log and open a new file handle
        time_t t = time(0);
        struct tm * now = localtime(&t);

        char date[21];
        strftime(date, 21, "%Y%m%d_%H%M%S", now);

        string fileNameWithoutExt;
        string fileExtension;
        HelperMethods helperMethods;
        stringstream fileNameStream;
        if (helperMethods.findFileNameAndExtensionFromFileName(StaticSettings::AppSettings::logFile, &fileNameWithoutExt,
                &fileExtension)) {
            fileNameStream << fileNameWithoutExt << "_" << date << "." << fileExtension;
        } else {
            fileNameStream << StaticSettings::AppSettings::logFile << "_" << date;
        }
        string archiveFileName;
        archiveFileName = fileNameStream.str();

        stringstream newPathStream;
        newPathStream << LogRotateConfiguration::archiveDirectoryName << "/" << archiveFileName;
        rename(StaticSettings::AppSettings::logFile.c_str(), newPathStream.str().c_str());

    }

    //Reopen the original log file or if it has been rotated, create a new log handle
    if (!StaticSettings::AppSettings::logFile.empty()) {
        logHandle->open(StaticSettings::AppSettings::logFile, ofstream::app);
		
    } else {
        //FATAL ERROR. Can't log, send SIGABRT as cannot continue
		cout << "Failed to log file - Fatal Error" << endl;
        raise(SIGABRT);
    }
}

/**
	Start the log rotation thread
*/
void LogRotation::startLogRotation() {
    logRotationMonitorThread = thread(&LogRotation::logRotationThread, this);

}

/**
	Deletes the oldest log file in the archive if required
*/
void LogRotation::logRotationThread() {
    LogRotation::logRotateThreadStarted = true;
	StatusManager statusManager;
    while (statusManager.getApplicationStatus() != StatusManager::ApplicationStatus::Stopping) {

        size_t directorySize = 0;
        string oldestFileName;
        time_t oldestDateFound = 0;

        namespace bf = boost::filesystem;
        for (bf::directory_iterator it(LogRotateConfiguration::archiveDirectoryName);
                it != bf::directory_iterator(); ++it) {
            directorySize += bf::file_size(*it);

            struct stat attrib;
            string fileName = it->path().filename().string();
            stringstream pathStream;
            pathStream << LogRotateConfiguration::archiveDirectoryName << "/" << fileName;
            string path = pathStream.str();


            stat(path.c_str(), &attrib);
            if (oldestDateFound == 0) {
                oldestDateFound = attrib.st_ctime;
                oldestFileName = fileName;
            } else if (attrib.st_ctime < (time_t) oldestDateFound) {
                oldestDateFound = attrib.st_ctime;
                oldestFileName = fileName;
            }
        }

        //Convert the directory size to MB
        directorySize = directorySize / 1024 / 1024;

        if (directorySize > (size_t) LogRotateConfiguration::maxArchiveSizeInMB) {

            stringstream pathStream;
            pathStream << LogRotateConfiguration::archiveDirectoryName << "/" << oldestFileName;
            remove(pathStream.str().c_str());
        }


        this_thread::sleep_for(chrono::seconds(LogRotateConfiguration::archiveSleepTimeInSeconds));
    }
    //If we get here then the thread is being stopped, therefore do not join the thread just let it finish
    logRotateShouldStop = true;
    cout << "Log rotation archive monitoring stopped. Current application status: " << statusManager.getApplicationStatus() << endl;
}

LogRotation::~LogRotation() {
    if (LogRotation::logRotateThreadStarted) {
        if (logRotationMonitorThread.joinable()) {
            logRotationMonitorThread.join();
        }
    }
}
