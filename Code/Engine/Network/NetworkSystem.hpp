#pragma once
#include <string>
#include <vector>
#include <deque>
#include <cstdint>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class	NamedStrings;
typedef NamedStrings EventArgs;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class NetworkState
{
	INACTIVE,
	IDLE,
	SERVER_LISTENING,
	CLIENT_CONNECTING,
	CLIENT_CONNECTED
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct NetworkConfig
{
	std::string m_hostAddress = "127.0.0.1";
	std::string m_hostPort = "3100";
	unsigned int m_maxClients = 4;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class NetworkSystem
{
public:

	NetworkSystem(NetworkConfig const& config);
	~NetworkSystem();
	
	void Startup();
	void Shutdown();
	void BeginFrame();
	void ServerReveiveAndSendFromClients();
	void ServerCheckAndAcceptNewConnections();
	void EndFrame();

	void StartServer();
	void StartClient(std::string serverIP = "127.0.0.1", std::string serverPort = "3100");
	void StopServer();
	void StopClient();
	void Disconnect(std::string const& message);

	void AddToSendBuffer(std::string const&			sendBufferString);
	void AddToSendBuffer(std::vector<char> const&	sendBufferData);
	std::vector<char> const& GetDataFromReceiveBuffer() const;

	NetworkState	GetCurrentNetworkState();
	std::string		GetIPAddress();
	std::string		GetNetPort();
	std::string		GetServerListenPortNumber();
	std::string		GetServerIPAddress();
	int				GetNumConnectedClients();

	static bool OnRemoteCmdReceived(EventArgs& args);

private:
	void ConnectToServer();

private:

	NetworkConfig m_config;
	NetworkState  m_state = NetworkState::INACTIVE;

	// #ToDo: May have to change this to something else
	uint64_t m_listenSocket = 0;
	uint64_t m_connectionToServer = 0;

	std::vector<uint64_t> m_connectedClientSockets;

	std::string m_hostIPAddress;
	std::string m_hostPort;

	std::vector<char> m_sendBuffer;
	std::vector<char> m_receiveBuffer;

};