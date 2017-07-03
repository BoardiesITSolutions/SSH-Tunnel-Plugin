#pragma once
#ifndef STATICSETTINGS_H
#define STATICSETTINGS_H
#include <iostream>
#include "INIParser.h"

class StaticSettings
{
public:
	StaticSettings(std::string configFiles);
	void readStaticSetting();
	struct AppSettings
	{
		static std::string logFile;
		static int minPortRange;
		static int maxPortRange;
		static int listenSocket;
		static bool debugJSONMessages;
		static int tunnelExpirationTimeInSeconds;
	};
private:
	std::string configFile;
	
};

#endif