#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2TCPIP.h>
#pragma comment(lib, "Ws2_32.lib")

#include "Engine/Network/NetworkSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Game/EngineBuildPreferences.hpp"

#if defined ENGINE_ENABLE_NETWORK
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
NetworkSystem* g_netSystem = nullptr;


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
NetworkSystem::NetworkSystem(NetworkConfig const& config)
	: m_config(config)
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
NetworkSystem::~NetworkSystem()
{
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::Startup()
{
	if(m_state != NetworkState::INACTIVE)
	{
		DebuggerPrintf("[Net System Startup] Failed to Startup net system. Network Already initialized\n");
		return;
	}

	WORD	winSockVersion = MAKEWORD(2, 2);
	WSADATA outData;

	int result = WSAStartup(winSockVersion, &outData);

	if(result != 0)
	{
		ERROR_AND_DIE(Stringf("Failed to Initialize Network Subsystem - Error Code: %d", result).c_str());
	}

	DebuggerPrintf("[Net System Startup] Net System Initialized\n");

	m_state = NetworkState::IDLE;

	SubscribeEventCallbackFunction("RemoteCmd", OnRemoteCmdReceived);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::Shutdown()
{
	UnsubscribeEventCallbackFunction("RemoteCmd", OnRemoteCmdReceived);

	if(m_state == NetworkState::CLIENT_CONNECTED)
	{
		StopClient();
	}

	if(m_state == NetworkState::SERVER_LISTENING)
	{
		StopServer();
	}

	int errorCode = WSACleanup();

	if(errorCode != 0)
	{
		errorCode = WSAGetLastError();
		ERROR_AND_DIE(Stringf("Cleanup failed. ErrorCode: %d", errorCode));
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::BeginFrame()
{
	m_receiveBuffer.clear();

	switch(m_state)
	{
		case NetworkState::INACTIVE:
			break;
		case NetworkState::IDLE:
			break;
		case NetworkState::SERVER_LISTENING:
		{
			ServerCheckAndAcceptNewConnections();

			if(static_cast<int>(m_connectedClientSockets.size()) <= 0)
			{
				break;
			}

			ServerReveiveAndSendFromClients();

			break;
		}
		case NetworkState::CLIENT_CONNECTING:
		{
			int result;

			fd_set writeSockets;
			fd_set exceptSockets;

			FD_ZERO(&writeSockets);
			FD_ZERO(&exceptSockets);

			FD_SET(static_cast<SOCKET>(m_connectionToServer), &writeSockets);
			FD_SET(static_cast<SOCKET>(m_connectionToServer), &exceptSockets);

			timeval waitTime = {};

			result = select(0, NULL, &writeSockets, &exceptSockets, &waitTime);

			if(result == SOCKET_ERROR)
			{
				DebuggerPrintf("[Client BeginFrame] Connection Failed...Attempting to reconnect\n");
				ConnectToServer();
				break;
			}

			result = FD_ISSET(static_cast<SOCKET>(m_connectionToServer), &exceptSockets);
			if(result != 0)
			{
				DebuggerPrintf("[Client BeginFrame] Attempting to reconnect...socket in error sockets\n");
				ConnectToServer();
				break;
			}

			result = FD_ISSET(static_cast<SOCKET>(m_connectionToServer), &writeSockets);
			if(result == 0)
			{
				DebuggerPrintf("[Client BeginFrame] Attempting to reconnect...socket not in write sockets\n");
				break;
			}

			// send and receive
			m_state = NetworkState::CLIENT_CONNECTED;

			break;
		}
		case NetworkState::CLIENT_CONNECTED:
		{
			int numBytesSent = send(static_cast<SOCKET>(m_connectionToServer), m_sendBuffer.data(), static_cast<int>(m_sendBuffer.size()), 0);

			if(numBytesSent == SOCKET_ERROR)
			{
				int wsaError = WSAGetLastError();

				if(wsaError != WSAEWOULDBLOCK)
				{
					ERROR_AND_DIE(Stringf("Socket Error while sending data (Client). Error Code: %d", wsaError).c_str());
				}
			}

			char tempRecvBuffer[2048];

			int numBytesReceived = recv(static_cast<SOCKET>(m_connectionToServer), tempRecvBuffer, 2048, 0);

			if(numBytesReceived == 0)
			{
				StopClient();
			}

			if(numBytesReceived == SOCKET_ERROR)
			{
				int wsaError = WSAGetLastError();
				if(wsaError == WSAEWOULDBLOCK)
				{
					break;
				}
				ERROR_AND_DIE(Stringf("Socket Error while receiving data (Client). Error Code: %d", wsaError).c_str());
			}

			m_receiveBuffer.insert(m_receiveBuffer.end(), tempRecvBuffer, tempRecvBuffer + numBytesReceived);

			break;
		}
		default:
			break;
	}

	m_sendBuffer.clear();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::ServerReveiveAndSendFromClients()
{

	for(int i = 0; i < static_cast<int>(m_connectedClientSockets.size()); ++i)
	{
		char tempRecvBuffer[2048];

		int numBytesReceived = recv(static_cast<SOCKET>(m_connectedClientSockets[i]), tempRecvBuffer, 2048, 0);

		if(numBytesReceived == 0)
		{
			closesocket(static_cast<SOCKET>(m_connectedClientSockets[i]));
			m_connectedClientSockets.erase(m_connectedClientSockets.begin() + i);
			i -= 1;
			continue;
		}

		if(numBytesReceived == -1)
		{
			continue;
		}

		if(numBytesReceived == SOCKET_ERROR)
		{
			int wsaError = WSAGetLastError();

			if(wsaError == WSAEWOULDBLOCK)
			{
				continue;
			}

			ERROR_AND_DIE(Stringf("Socket Error while receiving data (server). Error Code: %d", wsaError).c_str());
		}

		m_receiveBuffer.insert(m_receiveBuffer.end(), tempRecvBuffer, tempRecvBuffer + numBytesReceived);
	}

//	AddToSendBuffer(m_receiveBuffer);

	for(int i = 0; i < static_cast<int>(m_connectedClientSockets.size()); ++i)
	{
		int numBytesSent = send(static_cast<SOCKET>(m_connectedClientSockets[i]), m_sendBuffer.data(), static_cast<int>(m_sendBuffer.size()), 0);

		if(numBytesSent == SOCKET_ERROR)
		{
			int wsaError = WSAGetLastError();

			if(wsaError != WSAEWOULDBLOCK)
			{
				ERROR_AND_DIE(Stringf("Socket Error while sending data (server). Error Code: %d", wsaError).c_str());
			}
		}
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::ServerCheckAndAcceptNewConnections()
{
	SOCKET newClientSocket = INVALID_SOCKET;

	newClientSocket = accept(static_cast<SOCKET>(m_listenSocket), nullptr, nullptr);

	if(newClientSocket != INVALID_SOCKET)
	{
		unsigned long blockingMode = 1;
		ioctlsocket(newClientSocket, FIONBIO, &blockingMode);
		m_connectedClientSockets.push_back(newClientSocket);
		DebuggerPrintf("[Server BeginFrame] New Connection Request Accepted\n");
		FireEvent("PlayerJoined");
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::EndFrame()
{

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::StartServer()
{
	// create a socket
	m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(static_cast<SOCKET>(m_listenSocket) == INVALID_SOCKET)
	{
		ERROR_AND_DIE("Create Socket returned invalid socket");
	}

	unsigned long blockingMode = 1;

	// set listen socket to non blocking
	int result = ioctlsocket(static_cast<SOCKET>(m_listenSocket), FIONBIO, &blockingMode);

	if(result != 0)
	{
		int wsaError = WSAGetLastError();

		ERROR_AND_DIE(Stringf("IOCTL Socket did not return 0. WSA Error: %d", wsaError));
	}

	// bind server listen to a port
	uint32_t ipAddressU32 = INADDR_ANY;

	uint16_t listenPortU16 = static_cast<unsigned short>(atoi("3100"));

	sockaddr_in addr;
	addr.sin_family = AF_INET;

	addr.sin_addr.S_un.S_addr = htonl(ipAddressU32);

	addr.sin_port = htons(listenPortU16);

	result = bind(static_cast<SOCKET>(m_listenSocket), (sockaddr*)&addr, static_cast<int>(sizeof(addr)));

	if(result != 0)
	{
		int wsaError = WSAGetLastError();

		ERROR_AND_DIE(Stringf("Bind did not return 0. WSA Error: %d", wsaError));
	}

	result = listen(static_cast<SOCKET>(m_listenSocket), SOMAXCONN);

	if(result != 0)
	{
		int wsaError = WSAGetLastError();

		ERROR_AND_DIE(Stringf("Listen did not return 0. WSA Error: %d", wsaError));
	}

	m_state = NetworkState::SERVER_LISTENING;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::StartClient(std::string serverIP, std::string serverPort)
{
	m_config.m_hostAddress = serverIP;
	m_config.m_hostPort = serverPort;

	m_connectionToServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	unsigned long blockingMode = 1;
	int result = ioctlsocket(static_cast<SOCKET>(m_connectionToServer), FIONBIO, &blockingMode);

	if(result != 0)
	{
		int wsaError = WSAGetLastError();
		ERROR_AND_DIE(Stringf("IOCTL Socket for StartClient did not return 0. WSA Error: %d", wsaError));
	}

	ConnectToServer();

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::StopServer()
{
	for(int index = 0; index < static_cast<int>(m_connectedClientSockets.size()); ++index)
	{
		int result = closesocket(static_cast<SOCKET>(m_connectedClientSockets[index]));

		if(result == SOCKET_ERROR)
		{
			int wsaError = WSAGetLastError();
			ERROR_AND_DIE(Stringf("Close Socket failed for Socket %d. WSAError Code: %d", index, wsaError));
		}
	}

	m_connectedClientSockets.clear();

	int result = closesocket(static_cast<SOCKET>(m_listenSocket));

	if(result == SOCKET_ERROR)
	{
		int wsaError = WSAGetLastError();
		ERROR_AND_DIE(Stringf("Close Socket server listen socket. WSAError Code: %d", wsaError));
	}

	m_listenSocket = 0;

	m_state = NetworkState::IDLE;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::StopClient()
{
	closesocket(static_cast<SOCKET>(m_connectionToServer));
	m_state = NetworkState::IDLE;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::Disconnect(std::string const& message)
{

	UNUSED(message)

	switch(m_state)
	{
		case NetworkState::INACTIVE:
			break;
		case NetworkState::IDLE:
		{
			g_devConsole->AddLine(DevConsole::WARNING, "No active connection found");
			break;
		}
		case NetworkState::SERVER_LISTENING:
		{
			StopServer();
			break;
		}
		case NetworkState::CLIENT_CONNECTING:
		{
			StopClient();
			break;
		}
		case NetworkState::CLIENT_CONNECTED:
		{
			StopClient();
			break;
		}
		default:
			break;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::AddToSendBuffer(std::string const& sendBufferData)
{
	for(char byte : sendBufferData)
	{
		m_sendBuffer.push_back(byte);
	}
	m_sendBuffer.push_back('\0');
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::AddToSendBuffer(std::vector<char> const& sendBufferData)
{
	if(m_receiveBuffer.size() == 0)
	{
		return;
	}

	bool nullTerminator = false;

	for(char byte : sendBufferData)
	{
		m_sendBuffer.push_back(byte);
		if(byte == '\0')
		{
			nullTerminator = true;
		}
	}

	if(!nullTerminator)
	{
		m_sendBuffer.push_back('\0');
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::vector<char> const& NetworkSystem::GetDataFromReceiveBuffer() const
{
	return m_receiveBuffer;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
NetworkState NetworkSystem::GetCurrentNetworkState()
{
	return m_state;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string NetworkSystem::GetIPAddress()
{
	return m_config.m_hostAddress;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string NetworkSystem::GetNetPort()
{
	return m_config.m_hostPort;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
int NetworkSystem::GetNumConnectedClients()
{
	return static_cast<int>(m_connectedClientSockets.size());
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool NetworkSystem::OnRemoteCmdReceived(EventArgs& args)
{	
	std::string command = args.GetValue("cmd", "none");

	if(command == "none")
	{
		return false;
	}

	std::map<std::string, std::string> keyValuePair = args.GetKeyValuePairs();

	command += " ";

	for(auto const& [key, value] : keyValuePair)
	{
		if(key == "cmd")
		{
			continue;
		}
		else
		{
			command += key + "=";
			command += value + " ";
		}
	}

	g_netSystem->AddToSendBuffer(command);

	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NetworkSystem::ConnectToServer()
{
	IN_ADDR addr;

	int result = inet_pton(AF_INET, m_config.m_hostAddress.c_str(), &addr);

	if(result != 1)
	{
		int wsaError = WSAGetLastError();
		ERROR_AND_DIE(Stringf("INET_PTON StartClient did not return 0. WSA Error: %d", wsaError));
	}

	uint32_t serverIPAddress = ntohl(addr.S_un.S_addr);
	uint16_t serverPortU16 = static_cast<unsigned short>(atoi(m_config.m_hostPort.c_str()));

	sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;

	sock_addr.sin_addr.S_un.S_addr = htonl(serverIPAddress);

	sock_addr.sin_port = htons(serverPortU16);

	result = connect(static_cast<SOCKET>(m_connectionToServer), (sockaddr*)(&sock_addr), static_cast<int>(sizeof(sock_addr)));
	if(result != 0)
	{
		int wsaError = WSAGetLastError();

		if(wsaError != static_cast<int>(WSAEWOULDBLOCK))
		{
			ERROR_AND_DIE(Stringf("Failed to start a connection attempt : Client. WSA Error: %d", wsaError));
		}
	}

	DebuggerPrintf("[Client NetSystem::ConnectToServer] Beginning attempts to connect to server\n");
	m_state = NetworkState::CLIENT_CONNECTING;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string NetworkSystem::GetServerIPAddress()
{
	if(m_state != NetworkState::SERVER_LISTENING)
	{
		return "Server Not Started";
	}

	// Since we're bound to INADDR_ANY, get the actual local IP
	char hostname[256];
	if(gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
	{
		return "127.0.0.1";
	}

	struct addrinfo hints, * result;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if(getaddrinfo(hostname, nullptr, &hints, &result) != 0)
	{
		return "127.0.0.1";
	}

	struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)result->ai_addr;

	//Convert to std::string
	char ipBuffer[INET_ADDRSTRLEN];
	std::string ipAddress = "127.0.0.1";
	if(inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ipBuffer, INET_ADDRSTRLEN) != nullptr)
	{
		ipAddress = ipBuffer;
	}

	freeaddrinfo(result);
	return ipAddress;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string NetworkSystem::GetServerListenPortNumber()
{
	if(m_state != NetworkState::SERVER_LISTENING)
	{
		return "Server Not Started";
	}

	struct sockaddr_in serverAddr;
	int addrLen = sizeof(serverAddr);

	if(getsockname(static_cast<SOCKET>(m_listenSocket), (struct sockaddr*)&serverAddr, &addrLen) == SOCKET_ERROR)
	{
		return "Socket Not Found";
	}

	uint16_t port = ntohs(serverAddr.sin_port);
	return std::to_string(port);
}
#endif