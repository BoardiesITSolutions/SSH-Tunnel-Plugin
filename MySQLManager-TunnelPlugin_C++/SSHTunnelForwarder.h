#pragma once
#ifndef SSHTUNNELFORWARDER_H
#define SSHTUNNELFORWARDER_H
#include <libssh2.h>
#include <iostream>
#include <iomanip>
#include "JSONResponseGenerator.h"
#include <string>
#include <mutex>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
struct hostent *gethostbyname(const char *name);
#endif

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include "Logger.h"

#ifndef INADDR_NONE
#define INADDR_NONE (in_addr_t)-1
#endif

using namespace std;

//Forward declarations;
//ChosenAuthMethod chosenAuthMethod;

class SSHTunnelForwarder
{
public:
	enum SupportedAuthMethods { AUTH_NONE = 0, AUTH_PASSWORD, AUTH_PUBLICKEY };
	enum ErrorStatus { SUCCESS, SYSTEM_FAULT, DNS_RESOLUTION_FAILED, SSH_CONNECT_FAILED };
	SSHTunnelForwarder() {};
	//SSHTunnelForwarder(const SSHTunnelForwarder&) = default;
	SSHTunnelForwarder(Logger *logger, int localListenPort);
	void setUsername(std::string username);
	void setPassword(std::string password);
	void setSSHHostnameOrIPAddress(std::string ipOrHostname);
	void setSSHPort(int port);
	void setMySQLHost(std::string mysqlHost);
	void setMySQLPort(int mysqlPort);
	void setAuthMethod(SupportedAuthMethods chosenAuthMethod);
	void setSSHPrivateKey(std::string sshPrivateKey);
	void setSSHPrivateKeyCertPassphrase(std::string sshPrivateKeyCertPassphrase);
	string connectToSSHAndFingerprint(ErrorStatus& error);
	void setFingerprintConfirmed(bool fingerprintConfirmed);
	
	bool authenticateSSHServerAndStartPortForwarding(std::string *response);
	void acceptAndForwardToMySQL();
	std::string getSSHHostnameOrIPAddress();
	
	void closeSSHSessions();
	int getLocalListenPort();
	
	
private:
	int socketBindRetryCount = 0;
	string getUsername();
	std::string getPassword();
	bool hasSessionBeenClosed = false;
	int getSSHPort();
	std::string getMySQLHost();
	int getMySQLPort();
	bool getFingerprintConfirmed();
	std::string getSSHPrivateKey();
	std::string getSSHPrivateKeyCertPassphrase();
	
	SupportedAuthMethods getAuthMethod();
	std::string setupPortForwarding();
	std::string username;
	std::string password;
	std::string sshHostnameOrIpAddress;
	int sshPort;
	std::string mysqlHost;
	int mysqlPort;
	int supportedAuthMethod;
	SupportedAuthMethods chosenAuthMethod;
	std::string sshPrivateKey;
	std::string sshPrivateKeyPassPhrase;
	bool fingerprintConfirmed;
	Logger *logger;
	std::string sshServerIP;
	int localListenPort;
	LIBSSH2_SESSION *session = NULL;
	LIBSSH2_CHANNEL *channel = NULL;
	char sockopt;
#ifdef _WIN32
	SOCKET listensock = INVALID_SOCKET;
	SOCKET forwardsock = INVALID_SOCKET;
#else
	int listensock = -1;
	int forwardsock = -1;
#endif
	char * shost;
	int sport;
	fd_set fds;
	struct timeval tv;
	int rc, i;
	ssize_t len, wr;
	char buf[16384];
	struct sockaddr_in sin;
	socklen_t sinlen;
	std::thread acceptAndForwardThread;
	static std::mutex sshForwarderMutex;
	static int publicKeyAuthComplete(LIBSSH2_SESSION *session, unsigned char **sig, size_t *sig_len,
		const unsigned char *data, size_t data_len, void **abstract);
#ifdef _WIN32
	SOCKET sshSocket = INVALID_SOCKET;
#else
	int sshSocket = -1;
#endif
};

#endif