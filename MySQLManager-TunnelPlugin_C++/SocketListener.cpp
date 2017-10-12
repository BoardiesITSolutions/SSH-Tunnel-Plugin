/**
	Creates the listening socket that the PHP API will use to send data to request SSH tunnelling. 
	The socket listener will start a new thread, and each client that connects will create a socket processing thread
*/

#include "SocketListener.h"

using namespace std;

mutex SocketListener::socketListenerMutex;

/**
	Instantiate the socket listener class
	@param logger The logger class to allow writing debug and status of work being done on the sockets
*/
SocketListener::SocketListener(Logger *logger)
{
#ifdef _WIN32
	socketManager = WindowsSocket(logger);
#else
	socketManager = LinuxSocket(logger);
#endif
	this->logger = logger;
}

/**
	Creates the new listening socket for the PHP API to connect to. Will also start the socket listener thread where new client connections will be accepted
*/
void SocketListener::startSocketListener()
{
    try
    {
		if (!this->socketManager.createSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
			StaticSettings::AppSettings::listenSocket, 1024, "127.0.0.1"))
		{
			this->logger->writeToLog("Failed to prepare socket. Cannot continue");
			return;
		}
	
		if (!this->socketManager.bindAndStartListening())
		{
			this->logger->writeToLog("Failed to bind socket. Retrying in 10 seconds");
			this_thread::sleep_for(chrono::seconds(10));
			this->startSocketListener();
		}
		this->threadStarted = true;
		this->threadSocketListener = thread(&SocketListener::socketListenerThread, this);
	
		this->logger->writeToLog("Server socket has been successfully opened", "SocketListener", "startSocketListener");
    }
    catch (SocketException ex)
    {
        stringstream logstream;
        logstream << "Failed to start socket listener. Error: " << ex.what();
        this->logger->writeToLog(logstream.str(), "SocketListener", "startSocketListener");
		this->logger->writeToLog("Failed to bind socket. Retrying in 10 seconds");
		this_thread::sleep_for(chrono::seconds(10));
		this->startSocketListener();
    }
    catch (std::exception ex)
    {
        stringstream logstream;
        logstream << "Failed to start socket listener. General Exception: " << ex.what();
        this->logger->writeToLog(logstream.str(), "SocketListener", "startSocketListener");
    }
}

/**
	This is the thread for the socket listener. The thread will block waiting for a new client connection, as soon as a new client connection is created,
	a new thread is created to the socket processor where the SSH tunnelling is setup.
*/
void SocketListener::socketListenerThread()
{
	StatusManager statusManager;
	while (statusManager.getApplicationStatus() != StatusManager::ApplicationStatus::Stopping)
	{
            try
            {
		sockaddr_in clientAddr;
                memset(&clientAddr, 0, sizeof(sockaddr_in));
#ifdef _WIN32
		SOCKET *clientSocket = socketManager.acceptClientAndReturnSocket(&clientAddr);
#else
		int *clientSocket = socketManager.acceptClientAndReturnSocket(&clientAddr);
#endif //!_WIN32

                
		//As the accept client is blocking, check to see if the application status is no longer running and if so, close the listener socket
		//and the client socket. There is probably a better method than this as it means if the app is put into shutdown mode, it won't close until
		//1 extra client has connected.
		if (statusManager.getApplicationStatus() == StatusManager::ApplicationStatus::Stopping)
		{
			this->socketManager.closeSocket(clientSocket);
			this->socketManager.closeSocket();
			return;
		}

                SocketListener::socketListenerMutex.lock();
		SocketProcessor socketProcessor(this->logger, &socketManager);
                //socketProcessor.processSocketData(&clientSocket);
		std::thread *socketProcessorThread = new thread(&SocketProcessor::processSocketData, &socketProcessor, clientSocket);
		processingThreadList.push_back(socketProcessorThread);
                this_thread::sleep_for(chrono::seconds(1));
                SocketListener::socketListenerMutex.unlock();
            }
            catch (SocketException ex)
            {
                stringstream logstream;
                logstream << "Failed in socket listener thread loop. Error: " << ex.what();
                this->logger->writeToLog(logstream.str(), "SocketListener", "socketListenerThread");
                break;
            }
            catch (std::exception ex)
            {
                stringstream logstream;
                logstream << "Failed in socket listener thread loop. General Exception: " << ex.what();
                this->logger->writeToLog(logstream.str(), "SocketListener", "socketLlistenerThread");
            }
	}
	this->logger->writeToLog("Server socket has closed down", "SocketListener", "socketListenerThread");
	this->threadStarted = false;
}

SocketListener::~SocketListener()
{
	for (std::vector<std::thread*>::iterator it = processingThreadList.begin(); it != processingThreadList.end(); ++it)
	{
		if ((*it)->joinable())
		{
			(*it)->join();
		}
	}

	//If we get here then all of the threads should have finished so we can delete them from the vector and delete the thread from the heap
	while (processingThreadList.size() > 0)
	{
            SocketListener::socketListenerMutex.lock();
		for (std::vector<std::thread*>::iterator it = processingThreadList.begin(); it != processingThreadList.end(); ++it)
		{
			cout << "Deleting thread" << endl;
			std::thread *socketThread = *it;
			delete socketThread;
			it = processingThreadList.erase(it);
			break;
		}
            SocketListener::socketListenerMutex.unlock();
	}
        
        if (this->threadSocketListener.joinable())
	{
		this->threadSocketListener.join();
	}
}