#pragma once
#ifndef JSONRESPONSEGENERATOR_H
#define JSONRESPONSEGENERATOR_H

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <map>


class JSONResponseGenerator
{
public:
	JSONResponseGenerator();
	enum APIResponse { API_SUCCESS, API_GENERAL_ERROR, API_AUTH_FAILURE, API_NOT_IMPLEMENTED, API_TUNNEL_ERROR };
	void generateJSONResponse(JSONResponseGenerator::APIResponse result, std::string message);
	void generateJSONResponse(JSONResponseGenerator::APIResponse result, std::string message, std::map<std::string, std::string> *jsondata);
	std::string getJSONString();
private:
	rapidjson::Document jsondoc;
	rapidjson::Value jsonvalue;
};

#endif