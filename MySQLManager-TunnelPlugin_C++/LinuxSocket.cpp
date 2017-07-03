/**
Manages the creation, listening for data and sending data on a network socket for the Linux platforms
*/

#include "LinuxSocket.h"

#ifndef _WIN32

using namespace std;

/**
	Instantiates the LinuxSocket class, this class extends the BaseSocket class
	@param logger The initialised logger class from main.c which allows this class to write debug and socket events to the log file
*/
LinuxSocket::LinuxSocket(Logger *logger) :
    BaseSocket(logger)
{ }

/**
	Create a new socket, as the IP address is not specified in this call, the socket will be created to bind to any IP address on your server/PC
	@param family The socket family that should be created, e.g. AF_INET
	@param socketType The type of the socket that should be created, e.g. SOCK_STREAM
	@param protocol The protocol of the socket, e.g. TCP or UDP
	@param port The port number that the socket should use
	@param bufferLength The length of the buffer that should be used, this is the amount of data that is received on the socket at a time
*/
bool LinuxSocket::createSocket(int family, int socketType, int protocol, int port, int bufferLength)
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
bool LinuxSocket::createSocket(int family, int socketType, int protocol, int port, int bufferLength, string ipAddress)
{
    try
    {
        this->ipAddress = ipAddress;
        BaseSocket::createsocket(port, bufferLength);
        this->serverSocket = socket(family, socketType, protocol);
        if (this->serverSocket < 0)
        {
            stringstream logstream;
            logstream << "Error opening socket. Most likely trying to bind to an ";
            logstream << "invalid IP or the port is already in use";
            logger->writeToLog(logstream.str(), "LinuxSocket", "createSocket");
            return false;
        }

        switch (family)
        {
            case AF_INET: {
                this->serv_addr = new sockaddr_storage();
                bzero((sockaddr*)this->serv_addr, sizeof(this->serv_addr));
                sockaddr_in *sin = reinterpret_cast<sockaddr_in*>(serv_addr);
                sin->sin_family = family;
                //sin->sin_addr.s_addr = INADDR_ANY;
                //If IP Address is NULL then set to IPADDR_ANY
                if (ipAddress.empty())
                {
                    sin->sin_addr.s_addr = INADDR_ANY;
                }
                else
                {
                    inet_pton(AF_INET, ipAddress.c_str(), &sin->sin_addr);
                    //sin->sin_addr.s_addr = inet_addr(ipAddress.c_str());
                }
                sin->sin_port = htons(port);
                break;
            }
            case AF_INET6: {
                this->serv_addr = new sockaddr_storage();
                bzero((sockaddr_in6*)this->serv_addr, sizeof(this->serv_addr));
                sockaddr_in6 *sin = reinterpret_cast<sockaddr_in6*>(serv_addr);
                bzero(sin, sizeof(*sin));
                sin->sin6_family = family;
                sin->sin6_port = htons(port);
                if (ipAddress.empty())
                {
                    sin->sin6_addr = IN6ADDR_ANY_INIT;
                }
                else if (ipAddress == "::1")
                {
                    inet_pton(AF_INET6, ipAddress.c_str(), &(sin->sin6_addr));
                }
                else
                {
                    throw SocketException("Can only bind ipv6 via loopback or any interface");
                }
                
                break;
            }
            default:
                this->logger->writeToLog("Invalid socket family. Only AF_INET or AF_INET6 is supported");
                return false;
        }
        return true;
    }
    catch (exception ex)
    {
        stringstream logstream;
        logstream << "Failed to create a socket. Exception: " << ex.what();
        this->logger->writeToLog(logstream.str(), "LinuxSocket", "createSocket");
        return false;
    }
}

/**
	Bind and start listening on the socket using the platform default backlog setting.
*/
bool LinuxSocket::bindAndStartListening()
{
    stringstream logstream;
    int result = ::bind(this->serverSocket, (struct sockaddr *)this->serv_addr, sizeof(*this->serv_addr));
    if (result < 0)
    {
        logstream << "Failed to bind socket. Error: " << strerror(result);
        this->logger->writeToLog(logstream.str(), "LinuxSocket", "bindAndStartListening");
        throw SocketException(logstream.str().c_str());
        return false;
    }
    result = listen(this->serverSocket, this->socketPort);
    if (result < 0)
    {
        logstream << "Failed to start listening. Socket Error: " << strerror(result);
        throw SocketException(logstream.str().c_str());
    }
    logstream << "Socket " << this->socketPort << " has been successfully bound";
    this->logger->writeToLog(logstream.str(), "LinuxSocket", "bindAndStartListening");
    return true;
}

/**
	Wait for and accept new clients. The socket that is created from the client connection is returned
	@param clientAddr A memset initialised sockaddr_in structure where the client information will be stored when a new client connects
	@return SOCKET A socket descriptor of the client connection
*/
int LinuxSocket::acceptClientAndReturnSocket(sockaddr_in *clientAddr)
{
    socklen_t clilen = sizeof(clientAddr);
    int clientSocket = accept(this->serverSocket, (struct sockaddr *)&clientAddr, &clilen);
    if (clientSocket < 0)
    {
        stringstream logstream;
        logstream << "Unable to accept client socket. Error: " << strerror(clientSocket);
        throw SocketException(logstream.str().c_str());
    }
    return clientSocket;
}

/**
	Send data on the specified socket
	@param socket The socket descriptor where the data should be sent
	@param dataToSend The actual string content of the data that is to be sent on the socket
	@return int The number of bytes that have been sent on the socket
	@throws SocketException If the sending of the socket fails
*/
int LinuxSocket::sendToSocket(int *socket, std::string dataToSend)
{
    dataToSend.append("\r\n");
    int sentBytes = write(*socket, dataToSend.c_str(), dataToSend.length());
    if (sentBytes < 0)
    {
        stringstream logstream;
        logstream << "Failed to write to socket. Error: " << strerror(sentBytes);
        this->logger->writeToLog(logstream.str(), "LinuxSocket", "sendToSocket");
        throw SocketException(strerror(sentBytes));
    }
    return sentBytes;
}

/**
	Wait and receive data on the specified socket
	@param socket The socket that should be receiving data
	@return string The string content of the data that was received on the socket
	@throws SocketException If an error occurred while receiving the socket an exception is thrown
*/
string LinuxSocket::receiveDataOnSocket(int *socket)
{
    string receiveData = "";
    char * temp = NULL;
    int bytesReceived = 0;
    do
    {
        bytesReceived = recv(*socket, this->buffer, this->bufferLength, 0);
        if (bytesReceived < 0)
        {
            stringstream logstream;
            logstream << "Failed to read data on socket. Error: " << strerror(bytesReceived);
            this->logger->writeToLog(logstream.str(), "LinuxSocket", "receiveDataOnSocket");
            this->closeSocket(socket);
            throw SocketException(strerror(bytesReceived));
        }
        //If we got here then we should be able to get some data
        temp = new char[bytesReceived + 1];
        strncpy(temp, this->buffer, bytesReceived);
        temp[bytesReceived] = '\0';
        receiveData.append(temp);
        delete[] temp;
        temp = NULL;
        memset(this->buffer, 0, this->bufferLength);
    } while (bytesReceived == this->bufferLength);
    
    return receiveData;
}

/**
	Returns a pointer to the socket that was created by this class in the createSocket method, this can be used as the socket parameter to sendToSocket if you need to send data back to the PHP API
*/
int LinuxSocket::returnSocket()
{
    return this->serverSocket;
}

/**
	Closing the socket without any parameter will shutdown the listen socket that was created using the createSocket method
 */
void LinuxSocket::closeSocket()
{
    this->closeSocket(&this->serverSocket);
    
    if (this->serv_addr != NULL)
    {
        delete this->serv_addr;
    }
    if (this->buffer != NULL)
    {
        delete[] this->buffer;
    }
    this->serverSocket = -1;
}

/**
	Close the passed in socket. This would usually be the client socket. If you want to shutdown the main listening socket call this method without any parameters
	@param socket The client socket that should be closed
 */
void LinuxSocket::closeSocket(int *socket)
{
    if (*socket != -1)
    {
        int result = close(*socket);
        if (result < 0)
        {
            stringstream logstream;
            logstream << "Failed to close socket. " << strerror(result);
            throw SocketException(logstream.str().c_str());
        }
    }
}

#endif //!_WIN32