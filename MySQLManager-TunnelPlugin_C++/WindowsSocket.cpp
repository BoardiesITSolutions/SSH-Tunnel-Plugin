/**
	Manages the creation, listening for data and sending data on a network socket for the Windows platforms
*/
#include "WindowsSocket.h"

using namespace std;

/**
	Instantiates the WindowsSocket class, this class extends the BaseSocket class
	@param logger The initialised logger class from main.c which allows this class to write debug and socket events to the log file
*/
WindowsSocket::WindowsSocket(Logger *logger) :
	BaseSocket(logger)
{

}

/**
	Create a new socket, as the IP address is not specified in this call, the socket will be created to bind to any IP address on your server/PC
	@param family The socket family that should be created, e.g. AF_INET
	@param socketType The type of the socket that should be created, e.g. SOCK_STREAM
	@param protocol The protocol of the socket, e.g. TCP or UDP 
	@param port The port number that the socket should use
	@param bufferLength The length of the buffer that should be used, this is the amount of data that is received on the socket at a time
*/
bool WindowsSocket::createSocket(int family, int socketType, int protocol, int port, int bufferLength)
{
	return this->createSocket(family, socketType, protocol, port, bufferLength, "");
}

/**
	Create a new socket
	@param family The socket family that should be created, e.g. AF_INET
	@param socketType The type of the socket that should be created, e.g. SOCK_STREAM
	@param protocol The protocol of the socket, e.g. TCP or UDP
	@param port The port number that the socket should use
	@param bufferLength The length of the buffer that should be used, this is the amount of data that is received on the socket at a time
	@param ipAddress The IP address that the socket should use to bind to
*/
bool WindowsSocket::createSocket(int family, int socketType, int protocol, int port, int bufferLength, string ipAddress)
{
	stringstream logstream;
	//Call the base method to do the prep work e.g. create the buffer
	BaseSocket::createsocket(port, bufferLength);

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		logstream << "WSAStartup failed with error: " << iResult;
		this->logger->writeToLog(logstream.str(), "WindowsSocket", "createSocket");
		logstream.clear();
		logstream.str(string());
		return false;
	}
	this->serverSocket = socket(family, socketType, protocol);
	this->serv_addr.sin_family = family;
	if (ipAddress.empty())
	{
		this->serv_addr.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		this->serv_addr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
	}
	this->serv_addr.sin_port = htons(port);

	return true;
}

/**
	Bind and start listening on the socket using the platform default backlog setting. 
*/
bool WindowsSocket::bindAndStartListening()
{
	return this->bindAndStartListening(SOMAXCONN);
}

/**
	Bind and start listening on the socket overriding the platform default backlog setting
	@param backlog The maximum queue length that the socket is allowed to created, if the queue length is full when a new connection is received, the client may receive a connection refused error
*/
bool WindowsSocket::bindAndStartListening(int backlog)
{
	stringstream logstream;

	//iResult = ::bind(this->serverSocket, result->ai_addr, (int)result->ai_addrlen);
	iResult = ::bind(this->serverSocket, (SOCKADDR *)&this->serv_addr, sizeof(this->serv_addr));
	if (iResult != 0)
	{
		logstream << "Socket binding failed with error: " << iResult;
		this->logger->writeToLog(logstream.str(), "WindowsSocket", "bindAndStartListening");
		logstream.clear();
		logstream.str(string());
		FreeAddrInfo(result);
		closesocket(this->serverSocket);
		WSACleanup();
		return false;
	}

	freeaddrinfo(result);
	iResult = listen(this->serverSocket, backlog);
	if (iResult == SOCKET_ERROR)
	{
		throw SocketException(this->getErrorStringFromErrorCode(WSAGetLastError()).c_str());
		return false;
	}
	logstream << "Socket has binded and is now listening";
	this->logger->writeToLog(logstream.str(), "WindowsSocket", "bindAndStartListening");
	return true;
}

/**
	Wait for and accept new clients. The socket that is created from the client connection is returned
	@param clientAddr A memset initialised sockaddr_in structure where the client information will be stored when a new client connects
	@return SOCKET A socket descriptor of the client connection
*/
SOCKET *WindowsSocket::acceptClientAndReturnSocket(sockaddr_in *clientAddr)
{
	SOCKET *clientSocket = new SOCKET();
	*clientSocket = INVALID_SOCKET;
	//sockaddr_in clientAddr;
	socklen_t sin_size = sizeof(struct sockaddr_in);
	*clientSocket = accept(this->serverSocket, (struct sockaddr*)clientAddr, &sin_size);
	return clientSocket;
}

/**
	Send data on the specified socket
	@param socket The socket descriptor where the data should be sent
	@param dataToSend The actual string content of the data that is to be sent on the socket
	@return int The number of bytes that have been sent on the socket
	@throws SocketException If the sending of the socket fails
*/
int WindowsSocket::sendToSocket(SOCKET *socket, string dataToSend)
{
	//dataToSend.append("\r\n");
	int sentBytes = send(*socket, dataToSend.c_str(), dataToSend.length(), 0);
	if (sentBytes == SOCKET_ERROR)
	{
		throw SocketException(this->getErrorStringFromErrorCode(WSAGetLastError()).c_str());
	}
	return sentBytes;
}

/**
	Returns a pointer to the socket that was created by this class in the createSocket method, this can be used as the socket parameter to sendToSocket if you need to send data back to the PHP API
*/
SOCKET *WindowsSocket::returnSocket()
{
	return &this->serverSocket;
}

/**
	Wait and receive data on the specified socket
	@param socket The socket that should be receiving data
	@return string The string content of the data that was received on the socket
	@throws SocketException If an error occurred while receiving the socket an exception is thrown
*/
std::string WindowsSocket::receiveDataOnSocket(SOCKET *socket)
{
	if (*socket != -1)
	{
		string receivedData = "";
		char *temp = NULL;
		int bytesReceived = 0;
		do
		{
			bytesReceived = recv(*socket, this->buffer, this->bufferLength, 0);
			if (bytesReceived == SOCKET_ERROR)
			{
				string socketError = this->getErrorStringFromErrorCode(WSAGetLastError()).c_str();

				stringstream logstream;
				logstream << "Failed to receive data on socket.The socket will now be closed and cleanup performed. Error: " << socketError;

				this->logger->writeToLog(logstream.str(), "WindowsSocket", "receiveDataOnSocket");
				closesocket(*socket);
				WSACleanup();
				throw SocketException(socketError.c_str());
				return "";
			}

			//If we got here, then we should be able to get some data
			temp = new char[bytesReceived + 1];
			//memset(&temp, 0, bytesReceived + 1);
			strncpy(temp, this->buffer, bytesReceived);
			temp[bytesReceived] = '\0'; //Add a null terminator to the end of the string
			receivedData.append(temp);
			temp = NULL;

			//Now clear the buffer ready for more data
			memset(this->buffer, 0, this->bufferLength);

		} while (bytesReceived == this->bufferLength && bytesReceived >= 0); //Keep going until the received bytes is less than the buffer length

		return receivedData;
	}
	else
	{
		stringstream logstream;
		logstream << "Can't receive on socket as already be closed";
		throw SocketException(logstream.str().c_str());
	}
}

/**
	Returns an error description of the error code returned from one of the Windows socket methods
	@param errorCode The error code returned from one of the Windows socket methods, e.g. bind method
	@return A string containing the message
*/
string WindowsSocket::getErrorStringFromErrorCode(int errorCode)
{
	switch (errorCode)
	{
	case WSANOTINITIALISED:
		return "WSAStartup has not been called";
	case WSAENETDOWN:
		return "The network subsystem has failed";
	case WSAEACCES:
		return "The requested address is a broadcast address, but the appropriate flag was not set. Call setsockopt with so SO_BRODCAST socket option to enable use of the broadcast address";
	case WSAEINTR:
		return "A blocking Windows socket 1.1. call was cancelled through WSACancelBlockingCall";
	case WSAEINPROGRESS:
		return "A blocking Windows socket 1.1 call is in progress, or the service provider is still processing a callback function";
	case WSAEFAULT:
		return "The buf parameter is not completely contained in a valid part of the user address space";
	case WSAENETRESET:
		return "The connection has been broken due to the keep-alive activity detecting a failure whilee the operation was in progress";
	case WSAENOBUFS:
		return "No buffer space is available";
	case WSAENOTCONN:
		return "The socket is not connected";
	case WSAENOTSOCK:
		return "The descriptor is not a socket";
	case WSAEOPNOTSUPP:
		return "MSG_OOB was specified, but the socket is not stream-style such as SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is undirectional and supports only receive operations";
	case WSAESHUTDOWN:
		return "The socket has been shutdown; it is not possible to send on a socket after shutdown has been invoked with how set to SD_SEND or SD_BOTH";
	case WSAEWOULDBLOCK:
		return "The socket is marked as nonblocking and the requested operation would block";
	case WSAEMSGSIZE:
		return "The socket is message orientated, the message is larger than the maximum supported by the underlying transport";
	case WSAEHOSTUNREACH:
		return "The remote host cannot be reached from this host at this time";
	case WSAEINVAL:
		return "The socket has not been bound with the bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE";
	case WSAECONNABORTED:
		return "The virtual circuit was terminated due to a timeout or other failure. The application should close the socket as it is no longer usable";
	case WSAETIMEDOUT:
		return "The connection has been dropped, because of a network failure or because the system on the other end went down without notice";

	default:
		//If you see this, either see if you can publish a fix to make a case for it to provide a proper error message back, or let us know via the GitHub issue tracker or via
		//our issue tracker at https://support.boardiesitsolutions.com with the error code you received, and any information that you might have that we can use to help us replicate the problem
		stringstream logstream;
		logstream << "An unknown errror has occurred with the socket. The error code received is: " << errorCode;
		return logstream.str();
	}
}

/**
	If you created a copy of the socket descriptior, and have updated the options of the socket, then you can pass in the updated socket which will update the listening socket object within this class
	@param The updated socket pointer
*/
void WindowsSocket::updateClassSocket(SOCKET *sock)
{
	this->serverSocket = *sock;
}

/**
	Closing the socket without any parameter will shutdown the listen socket that was created using the createSocket method
*/
void WindowsSocket::closeSocket()
{
	if (this->serverSocket != -1)
	{
		this->closeSocket(&this->serverSocket);
		if (this->buffer != NULL)
		{
			delete[] this->buffer;
		}
		this->serverSocket = -1;
	}
}

/**
	Close the passed in socket. This would usually be the client socket. If you want to shutdown the main listening socket call this method without any parameters
	@param socket The client socket that should be closed
	@throws SocketException If for some reason the socket failed to be closed, a SocketException will be thrown
*/
void WindowsSocket::closeSocket(SOCKET *socket)
{
	if (*socket != -1)
	{
		int result = closesocket(*socket);
		delete socket;
		if (result < 0)
		{
			throw SocketException(this->getErrorStringFromErrorCode(result).c_str());
		}
	}
}