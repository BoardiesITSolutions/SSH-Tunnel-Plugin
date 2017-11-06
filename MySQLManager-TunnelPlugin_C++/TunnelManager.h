#pragma once
#ifndef TUNNELMANAGER_H
#define TUNNELMANAGER_H
#include <stdio.h>
#include <iostream>
#include <vector>
#include <rapidjson/document.h>
#include <mutex>
#include <stdlib.h>
#include "StaticSettings.h"
#include "Logger.h"
#include "StatusManager.h"

#include "JSONResponseGenerator.h"
#include <map>
#include <ctime>
#include "ActiveTunnels.h"
#include "SSHTunnelForwarder.h"
#ifdef _WIN32
#include "WindowsSocket.h"
#else
#include "LinuxSocket.h"
#endif

class TunnelManager
{

public:
	TunnelManager(Logger *logger);
	TunnelManager(Logger *logger, std::string json);
	bool startStopTunnel(void *socketManager, void *clientsockprt);
	void removeTunnelFromActiveList(int localPort);
	void tunnelMonitorThread();
	int findNextAvailableLocalSSHPort();
private:
	std::string postedFingerprint;
	SSHTunnelForwarder *sshTunnelForwarder = NULL;
	std::thread acceptAndForwardThread;
	enum TunnelCommand {CreateConnection, CloseConnection};
	enum AuthMethod {Password, PrivateKey};
	AuthMethod authMethod;
	static int currentListenPort;
	TunnelCommand tunnelCommand;
	std::string json;
	std::string sshUsername;
	std::string sshPassword;
	std::string privateKey;
	std::string certPassphrase;
	unsigned long long sshPort;
	std::string sshHost;
	unsigned long long remoteMySQLPort;
	std::string mysqlServerHost;
	int localPort;
	static std::vector<ActiveTunnels> activeTunnelsList;
	bool processJson();
	bool processTunnelCreation(rapidjson::Document& jsonObject);
	bool processTunnelClosure(rapidjson::Document& jsobObject);
	bool startTunnel(void *socketManager, void *clientsockptr);
	bool stopTunnel(void *socketManager, void *clientsockptr);
	static std::mutex tunnelMutex;
	bool doesPortExistInTunnel(int port);
	bool fingerprintConfirmed;
	int getFreePortCount();
	int getTotalPortCount();
	void setPostedFingerprint(std::string postedFingerprint);
	std::string getPostedFingerprint();
    void sendResponseToSocket(void * socketptr, void * socketManagerPtr, std::string jsonResponse);
	Logger *logger = NULL;

	//Setters
	void setAuthMethod(AuthMethod authMethod)
	{
		this->authMethod = authMethod;
	}
	void setTunnelCommand(TunnelCommand tunnelCommand)
	{
		this->tunnelCommand = tunnelCommand;
	}
	void setJson(std::string json)
	{
		this->json = json;
	}
	void setSSHUsername(std::string sshUsername)
	{
		this->sshUsername = sshUsername;
	}
	void setSSHPassword(std::string sshPassword)
	{
		this->sshPassword = sshPassword;
	}
	void setPrivateKey(std::string privateKey)
	{
		this->privateKey = PrivateKey;
	}
	void setCertPassphrase(std::string certPassphrase)
	{
		this->certPassphrase = certPassphrase;
	}
	void setSSHPort(int sshPort)
	{
		this->sshPort = sshPort;
	}
	void setSSHHost(std::string sshHost)
	{
		this->sshHost = sshHost;
	}
	void setRemoteMySQLPort(int remoteMySQLPort)
	{
		this->remoteMySQLPort = remoteMySQLPort;
	}
	void setMySQLServerHost(std::string mysqlServerHost)
	{
		this->mysqlServerHost = mysqlServerHost;
	}
	void setLocalPort(int localPort)
	{
		this->localPort = localPort;
	}
	void setFingerprintConfirmed(bool fingerprintConfirmed)
	{
		this->fingerprintConfirmed = fingerprintConfirmed;
	}

	//Getters
	AuthMethod getAuthMethod()
	{
		return this->authMethod;
	}
	TunnelCommand getTunnelCommand()
	{
		return this->tunnelCommand;
	}
	std::string getJson()
	{
		return this->json;
	}
	std::string getSSHUsername()
	{
		return this->sshUsername;
	}
	std::string getSSHPassword() 
	{
		return this->sshPassword;
	}
	std::string getPrivateKey()
	{
		return this->privateKey = privateKey;
	}
	std::string getCertPassphrase()
	{
		return this->certPassphrase = certPassphrase;
	}
	int getSSHPort()
	{
		return this->sshPort;	
	}
	std::string getSSHHost()
	{
		return this->sshHost;
	}
	int getRemoteMySQLPort()
	{
		return this->remoteMySQLPort;
	}
	std::string getMySQLServerHost()
	{
		return this->mysqlServerHost;
	}
	int getLocalPort()
	{
		return this->localPort;
	}
	bool getFingerprintConfirmed()
	{
		return this->fingerprintConfirmed;
	}
};
#endif
