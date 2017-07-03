	#ifndef INIPARSER_H

#define INIPARSER_H


#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include "HelperMethods.h"
using namespace std;

class INIParser
{
public:
	INIParser(string configFile);
	map<string, string> getSection(string sectionName);
	bool getKeyValueFromSection(string section, string key, string *value);
	bool getKeyValueFromSection(string section, string key, int *value);
	bool getKeyValueFromSection(string section, string key, long *value);
	bool getKeyValueFromSection(string section, string key, bool *value);
	bool doesMapKeyExist(map<string, string> *map, std::string key);
	~INIParser();
private:
	string configFile;
	ifstream fileHandle;
};
#endif // !INIPARSER_H