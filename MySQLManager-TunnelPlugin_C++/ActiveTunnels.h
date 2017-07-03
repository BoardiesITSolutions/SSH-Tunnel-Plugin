#pragma once
#ifndef ACTIVETUNNELS_H
#define ACTIVETUNNELS_H
#ifndef SSHTUNNELFORWARDER_H
#include "SSHTunnelForwarder.h"
#endif
#include "StaticSettings.h"
#include "StatusManager.h"

class ActiveTunnels
{
public:
	ActiveTunnels(SSHTunnelForwarder *sshTunnelForwarder, int localPort);
	SSHTunnelForwarder *sshTunnelForwarder;
	int localPort;
	time_t tunnelCreatedTime;
};


#endif