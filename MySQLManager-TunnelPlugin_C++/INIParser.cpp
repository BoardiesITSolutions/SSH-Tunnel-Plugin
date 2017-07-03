/**
	Read in the configuration file
*/

#include "INIParser.h"
#include "HelperMethods.h"

using namespace std;

/**
	Initialises the INIParser and opens a file handle to the passed in configuration file
	@param configFile The file name of the configuration file (note the config file has to be in the same location as the executable
	@throws ifstrea,::failure Throws exception if the file config file couldn't be opened
*/
INIParser::INIParser(string configFile)
{
	this->configFile = configFile;
	try
	{
		fileHandle.open(configFile.c_str());
	}
	catch (ifstream::failure ex)
	{
		throw ex;
	}
}

/**
	Gets the value of a specific key within a section within the configuration file
	@param section. The section name that should be retrieved - only provide the name not the brackets
	@param key The name of the key that should be looked up
	@param value A pointer to a int, the value of the key that is found within the config file - if it wasn't found the pointer is unchanged
	@return bool true on success or false on failure
*/
bool INIParser::getKeyValueFromSection(string section, string key, int *value)
{
	string temp;
	if (this->getKeyValueFromSection(section, key, &temp))
	{
		*value = stoi(temp);
		return true;
	}
	return false;
}

/**
Gets the value of a specific key within a section within the configuration file
@param section. The section name that should be retrieved - only provide the name not the brackets
@param key The name of the key that should be looked up
@param value A pointer to a long, the value of the key that is found within the config file - if it wasn't found the pointer is unchanged
@return bool true on success or false on failure
*/
bool INIParser::getKeyValueFromSection(string section, string key, long *value)
{
	string temp;
	if (this->getKeyValueFromSection(section, key, &temp))
	{
		*value = stol(temp);
		return true;
	}
	return false;
}

/**
Gets the value of a specific key within a section within the configuration file
@param section. The section name that should be retrieved - only provide the name not the brackets
@param key The name of the key that should be looked up
@param value A pointer to a bool, the value of the key that is found within the config file - if it wasn't found the pointer is unchanged
@return bool true on success or false on failure
*/
bool INIParser::getKeyValueFromSection(string section, string key, bool *value)
{
	string temp;
	if (this->getKeyValueFromSection(section, key, &temp))
	{
		if (temp == "true")
		{
			*value = true;
			return true;
		}
		else
		{
			*value = false;
			return true;
		}
	}
	return false;
}

/**
Gets the value of a specific key within a section within the configuration file
@param section. The section name that should be retrieved - only provide the name not the brackets
@param key The name of the key that should be looked up
@param value A pointer to a int, the value of the key that is found within the config file - if it wasn't found the pointer is unchanged
@return bool true on success or false on failure
*/
bool INIParser::getKeyValueFromSection(string section, string key, string *value)
{
	bool sectionFound = false;
	HelperMethods helperMethods;

	if (fileHandle.is_open())
	{
		string line;
		while (getline(fileHandle, line))
		{
			if (line.length() == 0)
			{
				continue;
			}

			if (line == "[" + section + "]")
			{
				sectionFound = true;
				continue;
			}
			if (sectionFound)
			{
				if (line.find_first_of('[') != string::npos)
				{
					//We're on a new section so break from loop
					sectionFound = false;
					continue;
				}
				else
				{
					vector<string> keyValues = helperMethods.splitString(line, '=');
					helperMethods.trimString(keyValues[0]);
					if (keyValues[0] == key)
					{
						//Restore the stream back to the start
						fileHandle.clear();
						fileHandle.seekg(0, ios::beg);
						helperMethods.trimString(keyValues[1]);
						*value = keyValues[1];
						return true;
						//return keyValues[1];
					}
				}
			}
		}
	}
	//Restore the stream back to the start
	fileHandle.clear();
	fileHandle.seekg(0, ios::beg);
	return false;
}

/**
	Reads in all the key/values from a specific section and returns map<string, string>
	@param section The section name that should be searched for, only include the section name, not the brackets
	@return map<string, string>
*/
map<string, string> INIParser::getSection(string section)
{
	bool sectionFound = false;
	map<string, string> keyValuePair;
	HelperMethods helperMethods;
	
	if (fileHandle.is_open())
	{
		string line;
		while (getline(fileHandle, line))
		{
			if (line.length() == 0)
			{
				continue;
			}
			//Is this the section we're looking for
			if (line == "[" + section + "]")
			{
				sectionFound = true;
				continue;
			}

			if (sectionFound)
			{
				if (line.find_first_of('[') != string::npos)
				{
					//We're on a new section so break from loop
					sectionFound = false;
					continue;
				}
				else
				{
					vector<string> keyValues = helperMethods.splitString(line, '=');
					
					helperMethods.trimString(keyValues[0]);
					helperMethods.trimString(keyValues[1]);

					keyValuePair[keyValues[0]] = keyValues[1];
				}
			}
		}
	}
	else
	{
		cout << "File no longer open" << endl;
	}
	//Restore the stream back to the start
	fileHandle.clear();
	fileHandle.seekg(0, ios::beg);
	return keyValuePair;
}

/**
	Check if the key exists within the map<string, string> which was returned in method INIParser::getSection(string section)
	@param lookup The map that contains the key/value pair which was returned in method INIParser::getSection(string section)
	@param key The key that should be searched for
	@bool Returns true if it was found, otherwise false was returned
*/
bool INIParser::doesMapKeyExist(map<string, string> *lookup, string key)
{
	map<string, string>::iterator iter = lookup->find(key);
	if (iter != lookup->end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

INIParser::~INIParser()
{
	if (fileHandle.is_open())
	{
		fileHandle.close();
	}
}