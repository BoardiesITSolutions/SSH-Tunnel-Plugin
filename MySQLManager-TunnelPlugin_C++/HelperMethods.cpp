/**
	Some static methods that do some common tasks that may be needed
*/
#include "HelperMethods.h"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

using namespace std;

static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

/**
	Base 64 encode a string
	@param buffer The text to be base64 encoded
	@param in_length The length of the text
	@return string A base64 encoded string
*/
string HelperMethods::base64Encode(const char* buffer, int in_len)
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
		char_array_3[i++] = *(buffer++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';

	}

	return ret;
}

/**
	Base64 decode a string
	@param encoded_string The base64 encoded string that should be decoded
	@return string The decoded version of the string
*/
string HelperMethods::base64Decode(string const& encoded_string)
{
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4) {
			for (i = 0; i <4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

	return ret;
}

/**
	Split a string by a particular character into a vector of string
	@param content The string content that should be split
	@char delimiter A char of what should be used to do the split
	@return vecor<string>
*/
vector<string> HelperMethods::splitString(string content, char delimiter)
{
	stringstream contentStream(content);
	string segment;
	vector<string> seglist;
	while (getline(contentStream, segment, delimiter))
	{
		seglist.push_back(segment);
	}
	return seglist;
}

/**
	Trim the given string to remove white space
	@param s The string that should be trimmed, the passed in string is modified, a copy is not returned. 
*/
void HelperMethods::trimString(string& s) {
	while (s.compare(0, 1, " ") == 0)
		s.erase(s.begin()); // remove leading whitespaces
	while (s.size()>0 && s.compare(s.size() - 1, 1, " ") == 0)
		s.erase(s.end() - 1); // remove trailing whitespaces
}

/**
	Find the filename and the extension of the passed in path
	@param fileName The path and/or filename  where the filename and extension should be returned
	@param fileNameWithoutExt A pointer to the string, if the method is succesful, then this will contain just file name but no extension
	@param extension A pointer to the string, if the method is successful, then this will contain just the exnteion, without the dot. 
	@return bool Returns true if both the filename and the extension was found, otherwise false is returned
*/
bool HelperMethods::findFileNameAndExtensionFromFileName(const string fileName, string *fileNameWithoutExt, string *extension)
{
	size_t extensionStart = fileName.find('.');
	if (extensionStart == string::npos)
	{
		*fileNameWithoutExt = "";
		*extension = "";
		return false; //No full stop so no extension, so return false and set the return points to an empty string
	}

	//A full stop was found so we can extract the extension
	*fileNameWithoutExt = fileName.substr(0, extensionStart);
	*extension = fileName.substr(extensionStart + 1);
	return true;
}

/**	
	Check if the passed in directory path exists on the file system
	@param directoryPath the path that should be checked if it exists
	@return boolean false if the directory does not exist, otherwise true
*/
bool HelperMethods::doesDirectoryExist(string directoryPath)
{
	struct stat info;
	if (stat(directoryPath.c_str(), &info) != 0)
	{
		//Directory cannot be access
		return false;
	}
	else if (info.st_mode & S_IFDIR)
	{
		return true;
	}
	else
	{
		return false;
	}
}