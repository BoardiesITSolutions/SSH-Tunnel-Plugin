/**
	Builds a JSON response in the required format for the PHP API
	If building a new method within the PHP API which communicates with this plugin,
	ensure that the response is returned by using this class. If you try doing a change 
	to this plugin and the PHP API that does not match this, it will be rejected. If you feel that
	this is required, then please get in touch with Boardies IT Solutions to discuss.
*/

#include "JSONResponseGenerator.h"

using namespace std;
using namespace rapidjson;

JSONResponseGenerator::JSONResponseGenerator()
{

}

/**
	Return a standard response with just the result and a message. 
	@param result This is the result of the request, if adding a new APIResponse to the enum, ensure that the enum value (API_SUCCESS = 0) matches with the defines in the PHP API
	@param message The message that is returned, this will usually be a camcelCase message that is then checked within the PHP API to do something, e.g. connectedFailed. The message isn't needed set it to a blank string (NOT NULL)
*/
void JSONResponseGenerator::generateJSONResponse(JSONResponseGenerator::APIResponse result, string message)
{
	map<string, string> data;
	this->generateJSONResponse(result, message, &data);
}

/**
	Return a standard response with just the result and a message. 
	@param result This is the result of the request, if adding a new APIResponse to the enum, ensure that the enum value (API_SUCCESS = 0) matches with the defines in the PHP API
	@param message The message that is returned, this will usually be a camcelCase message that is then checked within the PHP API to do something, e.g. connectedFailed. The message isn't needed set it to a blank string (NOT NULL)
	@param data A map<string,string> of key/value that are added to the response. This can be used in the event that extra data needs to be returned back to the PHP API
*/
void JSONResponseGenerator::generateJSONResponse(JSONResponseGenerator::APIResponse result, string message, map<string, string> *data)
{
	this->jsondoc.SetObject();
	rapidjson::Document::AllocatorType& allocator = this->jsondoc.GetAllocator();
	this->jsondoc.AddMember("result", result, allocator);
	Value v(message.c_str(), allocator);
	this->jsondoc.AddMember("message", v, allocator);
	
	if (data->size() > 0)
	{
		typedef map<string, string>::iterator it_type;
		for (it_type iterator = data->begin(); iterator != data->end(); iterator++)
		{
			Value k(iterator->first.c_str(), allocator);
			Value v(iterator->second.c_str(), allocator);
			this->jsondoc.AddMember(k, v, allocator);
		}
	}
}

/**
	Return the created JSON response (created using one of the two methods above), this JSON string is then passed back to the PHP API
	@returns the JSON string
*/
string JSONResponseGenerator::getJSONString()
{
	rapidjson::StringBuffer buffer;
	buffer.Clear();
	rapidjson::Writer<rapidjson::StringBuffer>writer(buffer);
	this->jsondoc.Accept(writer);
	return string(buffer.GetString());
}
