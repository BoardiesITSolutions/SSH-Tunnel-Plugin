/**
	Processes messages to and from the PHP API to set up the SSH tunnel
*/

#include "SocketProcessor.h"

using namespace std;
using namespace rapidjson;

/**
	Initiates the socket process class
	@param logger Allows logging debug events and any error messages during the processing of the socket data
	@param socketManager A pointer to the SocketManager class that is created in the SocketListener class
*/
SocketProcessor::SocketProcessor(Logger *logger, void *socketManager)
{
	this->logger = logger;
#ifdef _WIN32
	this->socketManager = static_cast<WindowsSocket*>(socketManager);;
#else
	this->socketManager = static_cast<LinuxSocket *>(socketManager);
#endif
}

/**
	Process the data that is received on the socket so that the SSH tunnel can be created
	@param clientPointer The socket descriptor that is returned in the acceptClientAndReturnSocket method which is called within the SocketListener class
*/
void SocketProcessor::processSocketData(void *clientpointer)
{
#ifdef _WIN32
	client = static_cast<SOCKET *>(clientpointer);
#else
	client = static_cast<int *>(clientpointer);
#endif
	try
	{
		string command = this->socketManager->receiveDataOnSocket(client);

		if (StaticSettings::AppSettings::debugJSONMessages)
		{
			//Replace the password with asterix so its not in the log
			rapidjson::Document jsonObject;
			jsonObject.Parse(command.c_str());

			string method = jsonObject["method"].GetString();

			//If it has the sshDetails object, then it may contain the password, if so, hide it, don't want SSH login credentials in the log file.
			if (jsonObject.HasMember("sshDetails"))
			{
				Value& sshDetails = jsonObject["sshDetails"];
				if (sshDetails.HasMember("sshPassword"))
				{
					string sshPassword = sshDetails["sshPassword"].GetString();

					for (unsigned int i = 0; i < sshPassword.length(); i++)
					{
						sshPassword[i] = '*';
					}

					rapidjson::Value::MemberIterator sshPasswordMember = sshDetails.FindMember("sshPassword");
					sshPasswordMember->value.SetString(sshPassword.c_str(), jsonObject.GetAllocator());
				}
				else if (sshDetails.HasMember("privateSSHKey"))
				{
					string privateKeyContent = sshDetails["privateSSHKey"].GetString();
					for (unsigned int i = 0; i < privateKeyContent.length(); i++)
					{
						privateKeyContent[i] = '*';
					}

					rapidjson::Value::MemberIterator privateKeyMember = sshDetails.FindMember("privateSSHKey");
					privateKeyMember->value.SetString(privateKeyContent.c_str(), jsonObject.GetAllocator());

					if (!sshDetails["certPassphrase"].IsNull())
					{
						string certPassphrase = sshDetails["certPassphrase"].GetString();
						for (unsigned int i = 0; i < certPassphrase.length(); i++)
						{
							certPassphrase[i] = '*';
						}

						rapidjson::Value::MemberIterator certPassphraseMember = sshDetails.FindMember("certPassphrase");
						certPassphraseMember->value.SetString(certPassphrase.c_str(), jsonObject.GetAllocator());
					}
					
				}
				else
				{
					//We should never get here, but if we do, log the command anyway. However, as this is potentially showing SSH login credentials, if you it come into here
					//and it is indeed show potential login details please raise the Issue on Github issue tracker or on our issue tracker at https://support.boardiesitsolutions.com
					//with details on how to re-product. Obviously though, when raising the issue, don't send us your login credentials :)
					logger->writeToLog(command);
				}
				//Convert it back to a string so that it can be logged
				rapidjson::StringBuffer buffer;
				buffer.Clear();
				rapidjson::Writer<rapidjson::StringBuffer>writer(buffer);
				jsonObject.Accept(writer);
				string jsonString = buffer.GetString();
				logger->writeToLog(jsonString);
			}
			else
			{ 
				logger->writeToLog(command);
			}
			
			
		}
		
		TunnelManager tunnelManager(this->logger, command);
		tunnelManager.startStopTunnel(socketManager, client);

		this->socketManager->closeSocket(client);
	}
	catch (SocketException ex)
	{
		if (strcmp(ex.what(), "The descriptor is not a socket") != 0 && strcmp(ex.what(), "Session already closed") != 0)
		{
			logger->writeToLog(ex.what(), "SocketProcessor", "processSocketData");
		}
		else
		{
			stringstream logstream;
			logstream << "Failed in processSocketData. Error: " << ex.what();
			logger->writeToLog(logstream.str(), "SocketProcessor", "processSocketData");
			//We don't need to worry about the following error as it just means the socket was closed while waiting to receive data
		}
	}
}
SocketProcessor::~SocketProcessor()
{
	if ((this->threadStarted) && this->socketProcessorThread.joinable())
	{
		this->socketProcessorThread.join();
	}
}