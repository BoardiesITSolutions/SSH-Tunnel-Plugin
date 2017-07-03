/**
	Responsible for setting up the SSH tunnelling, connecting to the SSH server, confirming the fingerprint and authenticating
*/

#include "SSHTunnelForwarder.h"
#include "TunnelManager.h"

using namespace std;
std::mutex SSHTunnelForwarder::sshForwarderMutex;

/**
	Initantiates the SSHTunnelForwarder class
	@param logger A pointer to the initialised logger class to allow debug and error events to be recorded in the log file while setting up the SSH tunnel
	@param localListenPort The local port that is being assigned to the current client connection for forwarding SSH tunnel traffic through
*/
SSHTunnelForwarder::SSHTunnelForwarder(Logger *logger, int localListenPort)
{
	this->logger = logger;
	this->localListenPort = localListenPort;
}

/**
* Getts and settters
*/
void SSHTunnelForwarder::setUsername(string username)
{
	this->username = username;
}
void SSHTunnelForwarder::setPassword(string password)
{
	this->password = password;
}
void SSHTunnelForwarder::setSSHHostnameOrIPAddress(string sshHostnameOrIpAddress)
{
	this->sshHostnameOrIpAddress = sshHostnameOrIpAddress;
}
void SSHTunnelForwarder::setSSHPort(int sshPort)
{
	this->sshPort = sshPort;
}
void SSHTunnelForwarder::setMySQLHost(string mysqlHost)
{
	this->mysqlHost = mysqlHost;
}
void SSHTunnelForwarder::setMySQLPort(int mysqlPort)
{
	this->mysqlPort = mysqlPort;
}
void SSHTunnelForwarder::setFingerprintConfirmed(bool fingerprintConfirmed)
{
	this->fingerprintConfirmed = fingerprintConfirmed;
}
void SSHTunnelForwarder::setAuthMethod(SupportedAuthMethods chosenAuthMethod)
{
	this->chosenAuthMethod = chosenAuthMethod;
}
void SSHTunnelForwarder::setSSHPrivateKey(string sshPrivateKey)
{
	this->sshPrivateKey = sshPrivateKey;
}
void SSHTunnelForwarder::setSSHPrivateKeyCertPassphrase(string certPassphrase)
{
	this->sshPrivateKeyPassPhrase = certPassphrase;
}

string SSHTunnelForwarder::getUsername()
{
	return this->username;
}
string SSHTunnelForwarder::getPassword()
{
	return this->password;
}
string SSHTunnelForwarder::getSSHHostnameOrIPAddress()
{
	return this->sshHostnameOrIpAddress;
}
int SSHTunnelForwarder::getSSHPort()
{
	return this->sshPort;
}
string SSHTunnelForwarder::getMySQLHost()
{
	return this->mysqlHost;
}
int SSHTunnelForwarder::getMySQLPort()
{
	return this->mysqlPort;
}
bool SSHTunnelForwarder::getFingerprintConfirmed()
{
	return this->fingerprintConfirmed;
}
SSHTunnelForwarder::SupportedAuthMethods SSHTunnelForwarder::getAuthMethod()
{
	return this->chosenAuthMethod;
}

string SSHTunnelForwarder::getSSHPrivateKey()
{
	return this->sshPrivateKey;
}
string SSHTunnelForwarder::getSSHPrivateKeyCertPassphrase()
{
	return this->sshPrivateKeyPassPhrase;
}

int SSHTunnelForwarder::getLocalListenPort()
{
	return this->localListenPort;
}

/**
	Connects to the SSH server and returns the SSH server finger print
	@param error Any errors are stored in this paramter
*/
string SSHTunnelForwarder::connectToSSHAndFingerprint(ErrorStatus& error)
{
	stringstream logstream;
	int rc, i;
	struct sockaddr_in sin;
	const char * tempfingerprint;

#ifdef WIN32
	char sockopt;
	
	SOCKET listensock = INVALID_SOCKET, forwardsock = INVALID_SOCKET;
	WSADATA wsadata;
	int err;

	err = WSAStartup(MAKEWORD(2, 0), &wsadata);
	if (err != 0) {
		fprintf(stderr, "WSAStartup failed with error: %d\n", err);
		error = ErrorStatus::SYSTEM_FAULT;
		return string();
	}
#endif

	//Convert the hostname to an ip address
	hostent *record = gethostbyname(this->sshHostnameOrIpAddress.c_str());
	if (record == NULL)
	{
#ifdef _WIN32
		logstream << "Unable to resolve " << this->sshHostnameOrIpAddress << " Error: " << WSAGetLastError();
#else
		logstream << "Unable to resolve " << this->sshHostnameOrIpAddress;
#endif
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "connectToSSHAndFingerprint");
		error = ErrorStatus::DNS_RESOLUTION_FAILED;
		return string();
	}

	in_addr * address = (in_addr *)record->h_addr;
	this->sshServerIP = inet_ntoa(*address);

	rc = libssh2_init(0);

	if (rc != 0) {
		logstream << "libssh2 initialised failed (" << rc << ")";
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "connectToSSHAndFingerprint");
		error = ErrorStatus::SYSTEM_FAULT;
		return string();
	}

	/* Connect to SSH server */
	this->sshSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef WIN32
	if (this->sshSocket == INVALID_SOCKET) {
		logstream << "Failed to open socket. Error: " << WSAGetLastError();
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "connectToSSHAndFingerprint");
		error = ErrorStatus::SYSTEM_FAULT;
		return string();
	}
#else
	if (this->sshSocket == -1) {
		logstream << "Failed to open socket. Error: " << strerror(this->sshSocket);
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "connectToSSHAndFingerprint");
		error = ErrorStatus::SYSTEM_FAULT;
		return string();
	}
#endif

	sin.sin_family = AF_INET;
	if (INADDR_NONE == (sin.sin_addr.s_addr = inet_addr(this->sshServerIP.c_str()))) {
		logstream << "Failed to copy server ip to structure";
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "connectToSSHAndFingerprint");
		error = ErrorStatus::SYSTEM_FAULT;
		return string();
	}
	sin.sin_port = htons(this->getSSHPort());

	sockopt = '1';
	setsockopt(this->sshSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&sockopt, sizeof(sockopt));

	int result = connect(this->sshSocket, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in));
	if (result != 0) {
#ifdef _WIN32
		logstream << "Failed to connect to SSH Server. Error: " << WSAGetLastError();
#else
		logstream << "Failed to connect to SSH Server. Error: " << strerror(result);
#endif
		this->logger->writeToLog(logstream.str(), "SSHTunnelForward", "connectToSSHAndFingerprint");
		error = ErrorStatus::SSH_CONNECT_FAILED;
		return string();
	}

	/* Create a session instance */
	this->session = libssh2_session_init();

	libssh2_trace(this->session, LIBSSH2_TRACE_PUBLICKEY | LIBSSH2_TRACE_ERROR | LIBSSH2_TRACE_AUTH);

	if (!this->session) {
		logstream << "Failed to initialise the SSH Session";
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "connectToSSHAndFingerprint");
		error = ErrorStatus::SYSTEM_FAULT;
		return string();
	}

	/* ... start it up. This will trade welcome banners, exchange keys,
	* and setup crypto, compression, and MAC layers
	*/
	rc = libssh2_session_handshake(this->session, this->sshSocket);

	if (rc) {
		logstream << "Error starting up SSH session. Error: " << rc;
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "connectToSSHAndFingerprint");
		error = ErrorStatus::SYSTEM_FAULT;
		return string();
	}

	/* At this point we havn't yet authenticated.  The first thing to do
	* is check the hostkey's fingerprint against our known.
	*/
	tempfingerprint = libssh2_hostkey_hash(this->session, LIBSSH2_HOSTKEY_TYPE_RSA);
	std::ostringstream fingerprintstream;
	fingerprintstream << std::hex << std::uppercase << std::setfill('0');
	for (i = 0; i < 16; i++)
	{
		fingerprintstream << std::setw(2) << (unsigned int)(tempfingerprint[i] & 0xFF) << ":";
	}
	string fingerprint = fingerprintstream.str();
	error = ErrorStatus::SUCCESS;
	return fingerprint.substr(0, fingerprint.size() - 1); //Remove the last colon (:) from the end of the string
}

/**
	Connects to the SSH server, authenticates and then sets up the SSH tunnel
	@param response This will be a JSON string generated by the JSONResponseGenerator class
	@return bool Returns true on success otherwise false
*/
bool SSHTunnelForwarder::authenticateSSHServerAndStartPortForwarding(string *response)
{

	char *userauthlist;
	userauthlist = libssh2_userauth_list(this->session, this->getUsername().c_str(), strlen(this->getUsername().c_str()));

	if (strstr(userauthlist, "password"))
	{
		supportedAuthMethod |= AUTH_PASSWORD;
	}
	if (strstr(userauthlist, "publickey"))
	{
		supportedAuthMethod |= AUTH_PUBLICKEY;
	}

	//Are we authenticating via a password
	if (this->getAuthMethod() == SupportedAuthMethods::AUTH_PASSWORD)
	{
		stringstream logstream;
		logstream << "Using password authentication for SSH Host: " << this->getSSHHostnameOrIPAddress();
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "authenticateSSHServer");
		if (chosenAuthMethod & SupportedAuthMethods::AUTH_PASSWORD)
		{
			if (libssh2_userauth_password(this->session, this->getUsername().c_str(), this->getPassword().c_str()) < 0)
			{
				logstream.clear();
				logstream.str(string());
				this->closeSSHSessions();
				logstream << "SSH Host " << this->getSSHHostnameOrIPAddress() << " password authentication failed";
				this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "authenticateSSHServer");
				JSONResponseGenerator jsonResponse;
				jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_TUNNEL_ERROR, "PasswordAuthFailed");
				*response =  jsonResponse.getJSONString();
				return false;
			}
			else
			{
				logstream.clear();
				logstream.str(string());
				logstream << "Successfully authenticated with SSH Host: " << this->getSSHHostnameOrIPAddress();
				this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "authenticateSSHServerAndStartPortForwarding");
			}
		}
		else
		{
			this->closeSSHSessions();
			logstream << "SSH Host: " << this->getSSHHostnameOrIPAddress() << " does not support password authentication";
			this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "authenticateSSHServer");
			JSONResponseGenerator jsonResponse;
			jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_AUTH_FAILURE, "PasswordAuthNotSupported");
			*response = jsonResponse.getJSONString();
			return false;
		}
	}
	else if (this->getAuthMethod() == SupportedAuthMethods::AUTH_PUBLICKEY)
	{

		string test = this->getSSHPrivateKey();

		//boost::replace_all(test, "\n", "");

		unsigned char * key = (unsigned char *)test.c_str();

		size_t sizeofkey = strlen((char*)key);
		cout << key << endl;
		stringstream logstream;
		logstream << "Using public key authentication for SSH Host: " << this->getSSHHostnameOrIPAddress();
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "authenticateSSHServer");
		if (chosenAuthMethod & SupportedAuthMethods::AUTH_PUBLICKEY)
		{
			//int result = 0;
			//int result = libssh2_userauth_publickey(this->session, this->getUsername().c_str(), key, sizeofkey, SSHTunnelForwarder::publicKeyAuthComplete, 0);
			string certpassPhrase = this->getSSHPrivateKeyCertPassphrase();
			int result;
			if (this->getSSHPrivateKeyCertPassphrase().empty())
			{
				result = libssh2_userauth_publickey_frommemory(this->session, this->getUsername().c_str(), strlen(username.c_str()), nullptr, 0, test.c_str(), sizeofkey, nullptr);
			}
			else
			{
				result = libssh2_userauth_publickey_frommemory(this->session, this->getUsername().c_str(), strlen(username.c_str()), nullptr, 0, test.c_str(), sizeofkey, this->getSSHPrivateKeyCertPassphrase().c_str());
			}
			if (result != 0)
			{
				char * error = NULL;
				int len = 0;
				int errbuf = 0;
				libssh2_session_last_error(this->session, &error, &len, errbuf);
				this->logger->writeToLog(std::string(error), "SSHTunnelForwarder", "auth");
				JSONResponseGenerator jsonResponse;
				if (result == -16) //Invalid certificate passphrase
				{
					jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_TUNNEL_ERROR, "KeyPassphraseError");
				}
				else if (result == -18) //Username and private key combination doesn't match
				{
					jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_TUNNEL_ERROR, "UsernameNotMatchedToPrivateKey");
				}
				else
				{
					jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_TUNNEL_ERROR, "InvalidPublicKey");
				}
				
				*response = jsonResponse.getJSONString();
				return false;
				
			}
		}
	}

	//At this point we've authenticated
	stringstream logstream;
	logstream << "SSH Host " << this->getSSHHostnameOrIPAddress() << " authenticated successfully";
	this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "authencateSSHServer");
	*response = this->setupPortForwarding();
	return true;
}

/**
	At this point the SSH server has successfully connected and authenticated, so now we need to set up the port forwarding so that 
	MySQL traffic can be tunnelled through the SSH server.
	@return string The JSON response generated by the JSONResponseGenerator class
*/
string SSHTunnelForwarder::setupPortForwarding()
{
	this->listensock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef WIN32
	if (this->listensock == INVALID_SOCKET)
	{
		stringstream logstream;
		logstream << "Failed to open listen socket";
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
		JSONResponseGenerator jsonResponse;
		jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_GENERAL_ERROR, "SocketCreationFailed");
		return jsonResponse.getJSONString();
	}
#else
	if (listensock == -1) {
		stringstream logstream;
		logstream << "Failed to open listen socket";
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
		JSONResponseGenerator jsonResponse;
		jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_GENERAL_ERROR, "SocketCreationFailed");
		return jsonResponse.getJSONString();
	}
#endif
	int sockopt = 1;
	setsockopt(this->listensock, SOL_SOCKET, SO_REUSEADDR, (char*)&sockopt, sizeof(sockopt));
	//setsockopt(this->listensock, SOL_SOCKET, SO_REUSEADDR, &this->sockopt, sizeof(this->sockopt));

	sin.sin_family = AF_INET;
	sin.sin_port = htons(this->localListenPort);
	if (INADDR_NONE == (sin.sin_addr.s_addr = inet_addr("127.0.0.1"))) {
		TunnelManager tunnelManager(this->logger);
		stringstream logstream;
		logstream << "Failed to set up local listen details";
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
		this->closeSSHSessions();
		JSONResponseGenerator jsonResponse;
		jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_GENERAL_ERROR, "LocalListenDetailsFailed");
		return jsonResponse.getJSONString();
	}
	sinlen = sizeof(sin);
	int result = ::bind(this->listensock, (struct sockaddr *)&sin, sinlen);
	if (result == -1) 
	{

		//The socket bind failed, probably because the local listen socket has only just closed - can take
		//a few seconds for the socket to clean up completely.
		//Get a new socket instead, but if it fails more than 3 times, actually send a failure so we don't
		//get stuck in a loop
		if (this->socketBindRetryCount < 3)
		{
			this->socketBindRetryCount++;
			stringstream logstream;
			logstream << "Local listen port " << this->localListenPort << " bind failed. Retrying ";
			logstream << this->socketBindRetryCount << " of 3";
#ifdef _WIN32
			logstream << "Bind Error: " << WSAGetLastError();
#else
			logstream << "Bind Error: " << strerror(result);
#endif
			this->closeSSHSessions();
			this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
			TunnelManager tunnelManager(this->logger);
			this->localListenPort = tunnelManager.findNextAvailableLocalSSHPort();
			logstream.clear();
			logstream.str(string());
			logstream << "Now got local listen port of " << this->localListenPort;
			this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
			return this->setupPortForwarding();
		}
		else
		{
			this->closeSSHSessions();
			stringstream logstream;
			logstream << "Failed to bind socket";
			this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
			JSONResponseGenerator jsonResponse;
			jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_GENERAL_ERROR, "SocketBindFailed");
			return jsonResponse.getJSONString();
		}
	}
	if (-1 == listen(listensock, SOMAXCONN)) {
		stringstream logstream;
		logstream << "Failed to start socket listening";
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
		this->closeSSHSessions();
		JSONResponseGenerator jsonResponse;
		jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_GENERAL_ERROR, "SocketListenFailed");
		return jsonResponse.getJSONString();
	}

	stringstream logstream;
	logstream << "Waiting for TCP connection on " << inet_ntoa(sin.sin_addr) << ":" << ntohs(sin.sin_port);
	this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarder");

	JSONResponseGenerator jsonResponse;
	map<string, string> data;
	data["LocalTunnelPort"] = std::to_string(this->localListenPort);
	jsonResponse.generateJSONResponse(JSONResponseGenerator::APIResponse::API_SUCCESS, "", &data);
	return jsonResponse.getJSONString();
}

/**
	The SSH tunnelling has been set up so now accept connections through the SSH tunnel
*/
void SSHTunnelForwarder::acceptAndForwardToMySQL()
{
	this->forwardsock = accept(listensock, (struct sockaddr *)&sin, &sinlen);
	if (this->hasSessionBeenClosed)
	{
#ifdef _WIN32
		closesocket(this->listensock);
#else
		close(this->listensock);
#endif
		return;
	}


	//The session is not null as we are re-using the session but accepting a new client so we need to 
	//be back in blocking mode for the setup.
	if (this->session != NULL)
	{
		libssh2_session_set_blocking(this->session, 1);
	}

#ifdef WIN32
	if (forwardsock == INVALID_SOCKET) {
		stringstream logstream;
		logstream << "Failed to accept forward socket. Error: " << WSAGetLastError();
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
		this->closeSSHSessions();
		return;
	}
#else
	if (forwardsock == -1) {
		stringstream logstream;
		logstream << "Failed to accept forward socket. Error: " << strerror(this->forwardsock);
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
		this->closeSSHSessions();
		return;
	}
#endif

	shost = inet_ntoa(sin.sin_addr);
	sport = ntohs(sin.sin_port);

	stringstream logstream;
	logstream << "Forwarding connection from " << shost << ":" << sport << " to ";
	logstream << this->getMySQLHost() << ":" << this->getMySQLPort();
	this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
	
	channel = libssh2_channel_direct_tcpip_ex(this->session, this->getMySQLHost().c_str(), this->getMySQLPort(), shost, sport);
	if (!channel) {
		char * error = NULL;
		int len = 0;
		int errbuf = 0;
		libssh2_session_last_error(this->session, &error, &len, errbuf);

		stringstream logstream;
		logstream << "Could not open the direct tcpip channel for port forwarding.";
		logstream << "Note that this could be a server problem so please check your SSH servers logs";
		logstream << "LIBSSH2 SSH Error: " << error;
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
		this->closeSSHSessions();
		return;
	}

	/* Must use non-blocking IO hereafter due to the current libssh2 API */
	libssh2_session_set_blocking(this->session, 0);

	while (1) {
		FD_ZERO(&fds);
		FD_SET(forwardsock, &fds);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		rc = select(forwardsock + 1, &fds, NULL, NULL, &tv);
		if (-1 == rc) {
			stringstream logstream;
#ifdef _WIN32
			logstream << "Socket select failed: Error: " << WSAGetLastError();
#else
			logstream << "Socket select failed. Error: " << strerror(rc);
#endif
			this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
			this->closeSSHSessions();
		}
		if (rc && FD_ISSET(forwardsock, &fds)) {
			len = recv(forwardsock, buf, sizeof(buf), 0);
			if (len < 0) {
				stringstream logstream;
#ifdef _WIN32
				logstream << "Failed to receive data on socket. Error: " << WSAGetLastError();
#else
				logstream << "Failed to receive data on socket. Error: " << strerror(len);
#endif
				this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
				this->closeSSHSessions();
				return;
			}
			else if (0 == len) 
			{
				stringstream logstream;
				logstream << "The client " << shost << ":" << sport << " has disconnected";
				this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
				libssh2_channel_send_eof(channel);
			libssh2_channel_close(channel);
			libssh2_channel_free(channel);
			channel = NULL;
#ifdef _WIN32
				closesocket(this->forwardsock);
				closesocket(this->listensock);
#else
				close(this->forwardsock);
				close(this->listensock);
#endif
				libssh2_channel_free(channel);
				channel = NULL;
				//this->acceptAndForwardToMySQL();
				this->closeSSHSessions();
				break;
			}
			wr = 0;
			while (wr < len) {
				i = libssh2_channel_write(channel, buf + wr, len - wr);

				if (LIBSSH2_ERROR_EAGAIN == i) {
					continue;
				}
				if (i < 0) {
					stringstream logstream;
					logstream << "libssh2_channel_write failed. Error: " << i;
					this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
					this->closeSSHSessions();
				}
				wr += i;
			}
		}
		while (1) 
		{
			if (this->hasSessionBeenClosed)
			{
				this->closeSSHSessions();
				return;
			}
			len = libssh2_channel_read(channel, buf, sizeof(buf));

			if (LIBSSH2_ERROR_EAGAIN == len)
				break;
			else if (len < 0) {
				stringstream logstream;
				logstream << "libssh2_channel_read failed. Error: " << (int)len;
				this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
				this->closeSSHSessions();
			}
			wr = 0;
			while (wr < len) {
				i = send(forwardsock, buf + wr, len - wr, 0);
				if (i <= 0) {
					stringstream logstream;
#ifdef _WIN32
					logstream << "Failed to send to forward socket. Error: " << WSAGetLastError();
#else
					logstream << "Failed to send to forward socket. Error: " << strerror(i);
#endif
					this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortforwarding");
					this->closeSSHSessions();
					return;
				}
				wr += i;
			}
			if (libssh2_channel_eof(channel))
			{
				stringstream logstream;
				logstream << "The server at " << this->getMySQLHost() << ":" << this->getMySQLPort();
				this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "setupPortForwarding");
				this->closeSSHSessions();
				return;
			}
		}
	}
}

/**
	Closes the current SSH session that is open and terminates any sockets that are being used by the SSH tunnel
*/
void SSHTunnelForwarder::closeSSHSessions()
{
	//SSHTunnelForwarder::sshForwarderMutex.lock();
	if (!hasSessionBeenClosed)
	{
		hasSessionBeenClosed = true;
		stringstream logstream;
		logstream << "Closing SSH session for host: " << this->getSSHHostnameOrIPAddress();
		this->logger->writeToLog(logstream.str(), "SSHTunnelForwarder", "closeSSHSessions");

#ifdef _WIN32
		if (this->forwardsock != INVALID_SOCKET)
		{
			closesocket(this->forwardsock);
		}

		if (this->listensock != INVALID_SOCKET)
		{
			closesocket(this->listensock);
		}

		if (this->sshSocket != INVALID_SOCKET)
		{
			closesocket(this->sshSocket);
		}
		
#else
		if (this->forwardsock != -1)
		{
                    //shutdown(this->forwardsock,SHUT_RDWR);
			close(this->forwardsock);
		}

		if (this->listensock != -1)
		{
                    //shutdown(this->listensock,SHUT_RDWR);
                        close(this->listensock);
		}
		if (this->sshSocket != -1)
		{
			close(this->sshSocket);
		}
#endif

		if (channel != NULL)
		{
			libssh2_channel_send_eof(channel);
			libssh2_channel_close(channel);
			libssh2_channel_free(channel);
			channel = NULL;
		}
		if (this->session != NULL)
		{
			libssh2_session_disconnect(this->session, "Client disconnecting normally");
			libssh2_session_free(this->session);
			this->session = NULL;
		}
		libssh2_exit();

		//Free the local port from the active tunnel list
		TunnelManager tunnelManager(this->logger);
		tunnelManager.removeTunnelFromActiveList(this->localListenPort);
	}
	else
	{
		cout << "Session already closed" << endl;
	}
	//SSHTunnelForwarder::sshForwarderMutex.unlock();
}