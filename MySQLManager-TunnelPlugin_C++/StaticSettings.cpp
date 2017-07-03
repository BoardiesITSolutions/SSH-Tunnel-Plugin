/**
	Reads in the configuration and stores the configuration in static variables that can be referenced without requiring to re-read the config file again
*/

#include "StaticSettings.h"

using namespace std;

int StaticSettings::AppSettings::minPortRange = 10000;
int StaticSettings::AppSettings::maxPortRange = 20000;
int StaticSettings::AppSettings::listenSocket = 500;
bool StaticSettings::AppSettings::debugJSONMessages = false;
int StaticSettings::AppSettings::tunnelExpirationTimeInSeconds = 5;
string StaticSettings::AppSettings::logFile = "";


/**
	Read in the configuration file and store the settings within the static variables
	@param configFile Path to the configuration file, only file name should be required as the configuration file should be with the executable
*/
StaticSettings::StaticSettings(string configFile)
{
	this->configFile = configFile;
}

void StaticSettings::readStaticSetting()
{
	INIParser iniParser(this->configFile);

	if (!iniParser.getKeyValueFromSection("app_settings", "minPortRange", &StaticSettings::AppSettings::minPortRange))
	{
		cout << "Failed to read minPortRange from [app_settings]. Defaulting to 10000" << endl;
		StaticSettings::AppSettings::minPortRange = 10000;
	}
	if (!iniParser.getKeyValueFromSection("app_settings", "maxPortRange", &StaticSettings::AppSettings::maxPortRange))
	{
		cout << "Failed to read maxPortRange from [app_settings]. Defaulting to 20000" << endl;
		StaticSettings::AppSettings::maxPortRange = 20000;
	}
	if (!iniParser.getKeyValueFromSection("app_settings", "listenSocket", &StaticSettings::AppSettings::listenSocket))
	{
		cout << "Failed to read listenSocket in [app_settings]. Defaulting to 500" << endl;
		StaticSettings::AppSettings::listenSocket = 500;
	}
	if (!iniParser.getKeyValueFromSection("app_settings", "debugXMLMessage", &StaticSettings::AppSettings::debugJSONMessages))
	{
		cout << "Failed to read debugXMLMessages in [app_settings]. Defaulting to false" << endl;
		StaticSettings::AppSettings::debugJSONMessages = false;
	}
	if (!iniParser.getKeyValueFromSection("app_settings", "tunnelExpirationTimeInSeconds", &StaticSettings::AppSettings::tunnelExpirationTimeInSeconds))
	{
		cout << "Failed to read tunnelExpirationTimeInSeconds in [app_settings]. Defaulting to 30 seconds" << endl;
		StaticSettings::AppSettings::tunnelExpirationTimeInSeconds = 30;
	}
}