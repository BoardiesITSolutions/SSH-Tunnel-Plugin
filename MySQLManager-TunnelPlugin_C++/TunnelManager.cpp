/**
	Manages the active SSH tunnels that are currently running. Will use the SSHTunnelForwarder class for setting up the tunnels and will automatically close any tunnels 
	that have been left open for too long
*/

#include "TunnelManager.h"

using namespace std;
using namespace rapidjson;

vector<ActiveTunnels> TunnelManager::activeTunnelsList;
std::mutex TunnelManager::tunnelMutex;
int TunnelManager::currentListenPort = StaticSettings::AppSettings::minPortRange;

/**
	Create an instance of the tunnel manager. This constructor is only used for starting the tunnel manager monitoring thread to automatically close tunnels
	@param logger Allow any debug or events to be logged 
*/
TunnelManager::TunnelManager(Logger *logger)
{
	this->logger = logger;
}

/**
	Initialies a tunnel manager object within logging and JSON string so the JSON can be processed for starting/stopping tunnels
	@param logger Allow any debug or events to be logged
	@param json The json message from the PHP API that was retrieved in the SocketProcess class
*/
TunnelManager::TunnelManager(Logger *logger, string json)
{
	this->json = json;
	this->logger = logger;
}

string TunnelManager::getPostedFingerprint()
{
	return this->postedFingerprint;
}

void TunnelManager::setPostedFingerprint(string postedFingerprint)
{
	this->postedFingerprint = postedFingerprint;
}

/**
	Monitor the active tunnels that are running, if any have been running for longer than the maximum time specified in the configuration file, close the tunnel
	Note: this might mean that queries that are taking a long time to complete, may get cut off and therefore the app won't receive the result. Configure this as per your requirements
*/
void TunnelManager::tunnelMonitorThread()
{
	StatusManager statusManager;
	while (statusManager.getApplicationStatus() != StatusManager::ApplicationStatus::Stopping)
	{
		SSHTunnelForwarder *sshTunnelForwarder = NULL;
		for (vector<ActiveTunnels>::iterator it = activeTunnelsList.begin(); it != activeTunnelsList.end(); ++it)
		{
			//Get the current time
			time_t currentTime = std::time(nullptr);
			
			//Get the time difference
             time_t timeDifference = currentTime - (*it).tunnelCreatedTime;
			
			//Time has expired so close the SSH Session
			if (timeDifference >= StaticSettings::AppSettings::tunnelExpirationTimeInSeconds)
			{
				sshTunnelForwarder = it->sshTunnelForwarder;
				it = activeTunnelsList.erase(it);
				break;
			}
		}

		if (sshTunnelForwarder != NULL)
		{
			stringstream logstream;
			logstream << "Host: " << sshTunnelForwarder->getSSHHostnameOrIPAddress() << " on client port " << sshTunnelForwarder->getLocalListenPort() << " has expired. Disconnecting";
			this->logger->writeToLog(logstream.str(), "TunnelManager", "tunnelMonitorThread");

			logstream.clear();
			logstream.str(string());
			logstream << "Now " << this->getFreePortCount() << " of " << this->getTotalPortCount() << " available";
			this->logger->writeToLog(logstream.str(), "TunnelManager", "tunnelMonitorThread");

			sshTunnelForwarder->closeSSHSessions();
			delete sshTunnelForwarder;
			sshTunnelForwarder = NULL;
		}
		this_thread::sleep_for(chrono::seconds(StaticSettings::AppSettings::tunnelExpirationTimeInSeconds));
	}
}

/**
	This method uses the json data to determine whether the tunnel should be started, or whether the app has decided that the tunnel that it was using can now be closed
	@param socketManager A pointer to the socket class (WindowsSocket or LinuxSocket)
	@param clientsockptr The socket descriptor where the response needs to be sent
	@return bool False on error otherwise true is returned
*/
bool TunnelManager::startStopTunnel(void *socketManager, void *clientsockptr)
{
	processJson();
	if (this->tunnelCommand == TunnelCommand::CreateConnection)
	{
		return this->startTunnel(socketManager, clientsockptr);
	}
	else if (this->tunnelCommand == TunnelCommand::CloseConnection)
	{
		return this->stopTunnel(socketManager, clientsockptr);
	}
	return false;
}

/**
	Stop the tunnel, the localTunnelPort within the JSON determines what SSH tunnel needs to be closed
	@param socketManagerPtr A pointer to the socket class (WindowsSocket or LinuxSocket)
	@param clientsockptr The socket descriptor where the response needs to be sent
	@return bool False on error otherwise true is returned
*/
bool TunnelManager::stopTunnel(void *socketManagerptr, void *clientsockptr)
{
#ifdef _WIN32
	WindowsSocket * socketManager = static_cast<WindowsSocket*>(socketManagerptr);
	SOCKET *clientsock = static_cast<SOCKET*>(clientsockptr);
#else
	LinuxSocket * socketManager = static_cast<LinuxSocket*>(socketManagerptr);
	int *clientsock = static_cast<int*>(clientsockptr);
#endif
	//Find we need to find the SSHTunnelForwarder so that we can call close session
	stringstream logstream;
	logstream << "Requested tunnel closure on port: " << this->getLocalPort();
	this->logger->writeToLog(logstream.str(), "TunnelManager", "stopTunnel");
	for (std::vector<ActiveTunnels>::iterator it = activeTunnelsList.begin(); it != activeTunnelsList.end(); ++it)
	{
		ActiveTunnels activeTunnel = *it;
		if (activeTunnel.localPort == this->getLocalPort())
		{
			logstream.clear();
			logstream.str(string());
			logstream << "Closing SSH tunnel for host: " << it->sshTunnelForwarder->getSSHHostnameOrIPAddress() << " for port " << this->getLocalPort();
			this->logger->writeToLog(logstream.str(), "TunnelManager", "stopTunnel");
			SSHTunnelForwarder *sshTunnelForwarder = activeTunnel.sshTunnelForwarder;
			sshTunnelForwarder->closeSSHSessions();
			JSONResponseGenerator jsonResponse;
			jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_SUCCESS, "");
			socketManager->sendToSocket(clientsock, jsonResponse.getJSONString());
#ifndef _WIN32
                        //shutdown(*clientsock, SHUT_RDWR);
			socketManager->closeSocket(clientsock);
#endif
			socketManager->closeSocket(clientsock);
			return true;
		}
	}
	return false;
}

/**
	Start an SSH tunnel
	@param socketManagerPtr A pointer to the socket class (WindowsSocket or LinuxSocket)
	@param clientsockptr The socket descriptor where the response needs to be sent
	@return bool False on error otherwise true is returned
*/
bool TunnelManager::startTunnel(void *socketManagerptr, void *clientsockptr)
{
	this->localPort = findNextAvailableLocalSSHPort();

	//Set up the basic details to connect to the SSH Server
	sshTunnelForwarder = new SSHTunnelForwarder(this->logger, this->localPort);
	sshTunnelForwarder->setUsername(this->sshUsername);
	sshTunnelForwarder->setPassword(this->sshPassword);
	sshTunnelForwarder->setSSHHostnameOrIPAddress(this->sshHost);
	sshTunnelForwarder->setSSHPort(this->sshPort);
	sshTunnelForwarder->setMySQLHost(this->mysqlServerHost);
	sshTunnelForwarder->setMySQLPort(this->remoteMySQLPort);
	if (this->getAuthMethod() == AuthMethod::Password)
	{
		sshTunnelForwarder->setAuthMethod(SSHTunnelForwarder::SupportedAuthMethods::AUTH_PASSWORD);
	}
	else if (this->getAuthMethod() == AuthMethod::PrivateKey)
	{
		sshTunnelForwarder->setAuthMethod(SSHTunnelForwarder::SupportedAuthMethods::AUTH_PUBLICKEY);
		sshTunnelForwarder->setSSHPrivateKey(privateKey);
		sshTunnelForwarder->setSSHPrivateKeyCertPassphrase(certPassphrase);
	}
	else
	{
		//We shouldn't get here, but as there are no viable authentication throw an error back to the API
		stringstream logstream;
		logstream << "No valid authentication method was selected. Only password and public and private ";
		logstream << "key is acceppted";
		this->logger->writeToLog(logstream.str(), "TunnelManager", "startTunnel");
		JSONResponseGenerator jsonResponse;
		jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_AUTH_FAILURE, "NoValidAuthMethod");
		string response = jsonResponse.getJSONString();
		logstream.clear();
		logstream.str(string());
		logstream << "Sending Response: " << response;
		this->logger->writeToLog(logstream.str(), "TunnelManager", "startTunnel");
		this->sendResponseToSocket(clientsockptr, socketManagerptr, response);
		delete sshTunnelForwarder;
		return false;
	}
	SSHTunnelForwarder::ErrorStatus errorStatus;
	string fingerprint = sshTunnelForwarder->connectToSSHAndFingerprint(errorStatus);
	if (!fingerprint.empty() && errorStatus == SSHTunnelForwarder::ErrorStatus::SUCCESS)
	{
		stringstream logstream;
		logstream << "SSH Host " << this->getSSHHost() << " has Fingerprint of: " << fingerprint;
		this->logger->writeToLog(logstream.str(), "TunnelManager", "startTunnel");
		//Send the fingerprint back to Android and check with the user whether they want to accept this token
		if (!this->getFingerprintConfirmed())
		{
			map<string, string> jsonData;
			jsonData["fingerprint"] = fingerprint;
			JSONResponseGenerator jsonResponse;
			jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_SUCCESS, "", &jsonData);
			string response = jsonResponse.getJSONString();
			this->sendResponseToSocket(clientsockptr, socketManagerptr, response);
			sshTunnelForwarder->closeSSHSessions();
			delete sshTunnelForwarder;
			return true;
		}
		else if (fingerprint.compare(this->getPostedFingerprint()) != 0)
		{
			//The fingerprint doesn't match what was posted, so send back an error to the user
			//to ask to confirm that the fingerprint is for the server what they expected
			sshTunnelForwarder->closeSSHSessions();
			map<string, string> jsonData;
			jsonData["fingerprint"] = fingerprint;
			JSONResponseGenerator jsonResponse;
			jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_TUNNEL_ERROR, "FingerprintNotMatched", &jsonData);
			string response = jsonResponse.getJSONString();
			this->sendResponseToSocket(clientsockptr, socketManagerptr, response);
			delete sshTunnelForwarder;
			return false;
		}
		//Fingerprint confirmed so auth and set up the port forwarding
		string response;
		bool result = sshTunnelForwarder->authenticateSSHServerAndStartPortForwarding(&response);
		this->sendResponseToSocket(clientsockptr, socketManagerptr, response);
		if (!result)
		{
			delete sshTunnelForwarder;
			return false; //Error in the authentication so stop processing
		}

		Document jsonObject;
		jsonObject.Parse(response.c_str());

		if (jsonObject["result"].GetInt() == JSONResponseGenerator::APIResponse::API_SUCCESS)
		{
			ActiveTunnels activeTunnels(sshTunnelForwarder, this->localPort);
			activeTunnelsList.push_back(activeTunnels);

			logstream.clear();
			logstream.str(std::string());
			logstream << "Current ports available: " << this->getFreePortCount();
			this->logger->writeToLog(logstream.str(), "TunnelManager", "startTunnel");

			sshTunnelForwarder->acceptAndForwardToMySQL();
		}
		delete sshTunnelForwarder;
		return true;
	}
	else
	{
		//If we got here then something went wrong, check the error ErrorStatus for the type
		string response;
		JSONResponseGenerator jsonResponse;
		if (errorStatus == SSHTunnelForwarder::ErrorStatus::SYSTEM_FAULT)
		{
			this->logger->writeToLog("Failed to start tunnel. System Fault error occurred", "TunnelManager", "startTunnel");
			jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_TUNNEL_ERROR, "SSH_SystemFaultOccurred");
		}
		else if (errorStatus == SSHTunnelForwarder::ErrorStatus::DNS_RESOLUTION_FAILED)
		{
			jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_TUNNEL_ERROR, "DNSResolutionFailed");
		}
		else if (errorStatus == SSHTunnelForwarder::ErrorStatus::SSH_CONNECT_FAILED)
		{
			this->logger->writeToLog("Failed to start tunnel. SSH Connect Failed", "TunnelManager", "startTunnel");
			jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_TUNNEL_ERROR, "SSHConnectFailed");
		}
		else
		{
			this->logger->writeToLog("Failed to start tunnel", "TunnelManager", "startTunnel");
			jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_TUNNEL_ERROR, "StartTunnelFailed");
		}
		response = jsonResponse.getJSONString();
		this->sendResponseToSocket(clientsockptr, socketManagerptr, response);
		delete sshTunnelForwarder;
		return false;
	}

}

/**
	Using the min and max port range from the config file, find the next available local port to use to tunnel SSH traffic
	@return int The local port number that is to be used
*/
int TunnelManager::findNextAvailableLocalSSHPort()
{
	tunnelMutex.lock();
	do
	{ 
		if (TunnelManager::currentListenPort >= StaticSettings::AppSettings::maxPortRange)
		{
			stringstream logstream;
			logstream << TunnelManager::currentListenPort << " reach max value. Resetting the port range back to " << StaticSettings::AppSettings::minPortRange;
			this->logger->writeToLog(logstream.str(), "TunnelManager", "findNextAvailableLocalSSHPort");
			TunnelManager::currentListenPort = StaticSettings::AppSettings::minPortRange;
		}
	} while (TunnelManager::currentListenPort == 0);
	int port = TunnelManager::currentListenPort;
	TunnelManager::currentListenPort++;
	tunnelMutex.unlock();
	return port;
}

/**
	Check if the specific port number is already in use by an active SSH tunnel
	@param port The local port number that should be checked
	@return bool Return true if the port number is already being used, otherwise false is returned
*/
bool TunnelManager::doesPortExistInTunnel(int port)
{
	for (std::vector<ActiveTunnels>::iterator it = TunnelManager::activeTunnelsList.begin(); it != TunnelManager::activeTunnelsList.end(); ++it)
	{
		cout << "Does port exist. Iterator port contains: " << (it)->localPort << endl;
		if ((it)->localPort == port)
		{
			return true;
		}
	}
	return false;
}

/**
	Process the JSON data, this data determines how the SSH tunnel is created, or whether an SSH tunnel should be stopped
	@return true on success otherwise false
*/
bool TunnelManager::processJson()
{
	Document jsonObject;
	jsonObject.Parse(this->json.c_str());

	string method = jsonObject["method"].GetString();

	//if (jsonObject["method"].GetString() == "CreateTunnel")
	if (std::string(jsonObject["method"].GetString()).compare("CreateTunnel") == 0)
	{
		this->tunnelCommand = TunnelCommand::CreateConnection;
		return this->processTunnelCreation(jsonObject);
	}
	else if (std::string(jsonObject["method"].GetString()).compare("CloseTunnel") == 0)
	{
		this->tunnelCommand = TunnelCommand::CloseConnection;
		return this->processTunnelClosure(jsonObject);
	}
	else
	{
		stringstream logstream;
		logstream << "Invalid JSON message was received. JSON was: " << this->json;
		this->logger->writeToLog(logstream.str(), "TunnelManager", "processJson");
		return false;
	}
	return true;
}

/**
	Set required class members in order to close the SSH tunnel
	@param jsonObject a reference to the json Document created in the processJson method
	@return bool True on success otherwise false
*/
bool TunnelManager::processTunnelClosure(Document& jsonObject)
{
	this->setLocalPort(jsonObject["localPort"].GetInt());
	return true;
}

/**
	Set required class memembers in order to open the SSH tunnel
	@param jsonObject a reference to the json Document created in the processJson method
	@return bool True on success otherwise false
*/
bool TunnelManager::processTunnelCreation(Document& jsonObject)
{
	//Determine the auth method
	const Value& sshDetails = jsonObject["sshDetails"];
	if (std::string(sshDetails["authMethod"].GetString()).compare("Password") == 0)
	{
		authMethod = AuthMethod::Password;
	}
	else
	{
		authMethod = AuthMethod::PrivateKey;
	}
	sshUsername = sshDetails["sshUsername"].GetString();
	if (authMethod == AuthMethod::Password)
	{
		sshPassword = sshDetails["sshPassword"].GetString();
	}
	else
	{
		privateKey = sshDetails["privateSSHKey"].GetString();
		if (sshDetails.HasMember("certPassphrase"))
		{
			if (!sshDetails["certPassphrase"].IsNull())
			{
				certPassphrase = sshDetails["certPassphrase"].GetString();
			}
		}
	}
	sshPort = sshDetails["sshPort"].GetInt();
	sshHost = sshDetails["sshHost"].GetString();
	remoteMySQLPort = jsonObject["remoteMySQLPort"].GetInt();
	mysqlServerHost = jsonObject["mysqlHost"].GetString();
	fingerprintConfirmed = jsonObject["fingerprintConfirmed"].GetBool();
	if (jsonObject.HasMember("fingerprint"))
	{
		postedFingerprint = jsonObject["fingerprint"].GetString();
	}
	return true;
}

/**
	Remove the SSH tunnel details from the active tunnel list. The tunnel should already have been closed before this gets called
	@param localPort The local port determines what SSH tunnel should be removed from the active tunnel list. This port is unique to each active tunnel
*/
void TunnelManager::removeTunnelFromActiveList(int localPort)
{
    TunnelManager::tunnelMutex.lock();
	for (std::vector<ActiveTunnels>::iterator it = activeTunnelsList.begin(); it != activeTunnelsList.end(); ++it)
	{
		ActiveTunnels activeTunnels = *it;
		if (activeTunnels.localPort == localPort)
		{
			it = activeTunnelsList.erase(it);
			break;
		}
	}
    TunnelManager::tunnelMutex.unlock();
	stringstream logstream;
	logstream << "Current ports available: " << this->getFreePortCount() << " of " << this->getTotalPortCount();
	this->logger->writeToLog(logstream.str(), "TunnelManager", "removeTunnelFromActiveList");
}

/**
	Return the full port range available using the max port range and min port range from the configuration
	@return int The total ports that are available for use (this is all ports within the range, this doesn't take into account what ports are in use)
*/
int TunnelManager::getTotalPortCount()
{
	return (StaticSettings::AppSettings::maxPortRange - StaticSettings::AppSettings::minPortRange);
}

/**
	Get the current free port count that are available, i.e. how many more SSH tunnels can be created
	@return int The total number of ports that are available for use
*/
int TunnelManager::getFreePortCount()
{
	return (this->getTotalPortCount() - activeTunnelsList.size());
}

/**
	Send the JSON response to a socket
	@param socketptr A socket descriptor for where the response should be sent
	@param socketManagerPtr A pointer to the WindowsSocket or LinuxSocket class depending on the platform being used. This class is response for sending the response
	@param jsonResponse The actual JSON response that is sent on the socket
*/
void TunnelManager::sendResponseToSocket(void * socketptr, void * socketManagerPtr, std::string jsonResponse)
{
	try
	{
#ifdef _WIN32
		SOCKET * clientsock = static_cast<SOCKET *>(socketptr);
		WindowsSocket *socketManager = static_cast<WindowsSocket*>(socketManagerPtr);
		int i = socketManager->sendToSocket(clientsock, jsonResponse);
#else
		int *clientsock = static_cast<int *>(socketptr);
		LinuxSocket *socketManager = static_cast<LinuxSocket *>(socketManagerPtr);
		socketManager->sendToSocket(clientsock, jsonResponse);
#endif
	}
	catch (SocketException ex)
	{
		stringstream logstream;
		logstream << "Failed to send json response to client socket. Error: " << ex.what();
		this->logger->writeToLog(logstream.str(), "TunnelManager", "sendResponseToSocket");
	}
}