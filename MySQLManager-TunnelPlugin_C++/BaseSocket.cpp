/**
	This class is the base class for socket handler classes, WindowsSocket.cpp and LinuxSocket.cpp
	This class can't be created directly, will only be instantiated when WindowsSocket or LinuxSocket is initialised
*/

#include "BaseSocket.h"

using namespace std;

/**
	@param logger The logger class to allow the socket handler classes mentioned above, be able to log the current state and debg
*/
BaseSocket::BaseSocket(Logger *logger)
{
	this->logger = logger;
}

/**
	This sets up the buffer length and socket port number being used, the WindowsSocket.cpp and LinuxSocket.cpp
	will do other stuff to create the port but this is the default port that each platform has to do. 
	@param port The socket port number that is going to be created. This is confiruable within tunnel.conf. This is used when creating the socket that is used by the PHP API. 
	@param bufferLength This is the length of the buffer for the socket, i.e. this is how many bytes will be retrieved from the socket at a time.
*/
bool BaseSocket::createsocket(int port, int bufferLength)
{
	this->bufferLength = bufferLength;
	this->buffer = new char[bufferLength];
	this->socketPort = port;

	stringstream logstream;
	logstream << "Creating buffer of length " << bufferLength;
	this->logger->writeToLog(logstream.str(), "BaseSocket", "createSocket");
	return true; //The base method can't fail but has to return soemthing
}