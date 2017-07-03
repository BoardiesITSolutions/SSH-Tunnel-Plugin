#include "ActiveTunnels.h"


using namespace std;

/**
	Container object for open SSH tunnels. These objects are stored within a list within the TunnelManager
	to ensure that SSH tunnels that have been open for too long are terminated
	@param sshTunnelForwarder The SSH tunnel forwarder class object, this is responsible for opening/closing and sending/receiving SSH data via the SSH socket
	@param localPort The local port that is being used for the SSH tunnel
*/
ActiveTunnels::ActiveTunnels(SSHTunnelForwarder *sshTunnelForwarder, int localPort)
{
	this->sshTunnelForwarder = sshTunnelForwarder;
	this->localPort = localPort;
	this->tunnelCreatedTime = std::time(nullptr);
}
