#ifndef BITS_HELPERMETHODS_H

#define BITS_HELPERMETHODS_H


#include <iostream>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

class HelperMethods
{
public:
	string base64Encode(const char* buffer, int in_len);
	string base64Decode(string const& encoded_string);
	vector<string> splitString(string content, char delimiter);
	void trimString(string& s);
	bool findFileNameAndExtensionFromFileName(const string fileName, string *fileNameWithoutExt, string *extension);
	bool doesDirectoryExist(string directPath);
};

#endif // !HELPERMETHODS_H