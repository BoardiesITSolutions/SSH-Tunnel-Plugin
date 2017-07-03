#pragma once
#ifndef LOGGER_H
#define LOGGER_H
#include <string>
#include "LogRotation.h"
#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>
#include <stdio.h>
#include <cstdlib>
#include "StaticSettings.h"
#include "INIParser.h"
#include "LogRotation.h"
#include "StatusManager.h"
#include <signal.h>
#include <stdio.h>
#include <cstdlib>



class Logger
{
public:
	Logger();
	~Logger();
	void writeToLog(std::string logLine);
	void writeToLog(std::string logLine, std::string className, std::string methodInfo);
private:
	static ofstream logHandle;
};

#endif