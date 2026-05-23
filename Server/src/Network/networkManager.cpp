#include "networkManager.h"
#include <WS2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

NetworkManager::NetworkManager()
    : m_ListeningSocket(INVALID_SOCKET) {
}

NetworkManager::~NetworkManager() {
    Shutdown();
}

bool NetworkManager::Initialize(int port)
{
    //Created socket
    SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == INVALID_SOCKET) {
        std::cout << "Cant create a socket";
        return false;
    }

    //Binding ip and port to a socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
    hint.sin_addr.S_un.S_addr = INADDR_ANY;

    bind(listening, (sockaddr*)&hint, sizeof(hint));

    //socket is for listening
    listen(listening, SOMAXCONN);

    //wait for connection
    sockaddr_in client;
    int clientSize = sizeof(client);

    SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
    if (clientSocket == INVALID_SOCKET)
    {

    }

    char host[NI_MAXHOST];      //Clients remote name
    char service[NI_MAXHOST];   //Service (port) the client is connected on

    ZeroMemory(host, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
    ZeroMemory(service, NI_MAXSERV);

    if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
    {
        std::cout << host << " connected to port " << service << std::endl;
    }
    else
    {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;
    }

    closesocket(listening);

    char buf[4096];

    //close socket
    closesocket(clientSocket);
    while (true)
    {
        ZeroMemory(buf, 4096);

        //Wait for Client to send data
        int bytesRecieved = recv(clientSocket, buf, 4096, 0);
        if (bytesRecieved == SOCKET_ERROR)
        {
            std::cout << "Error in recv()" << std::endl;
            break;
        }

        if (bytesRecieved == 0) {
            std::cout << "Client Disconnected " << std::endl;
            break;
        }

        // Echo message back to client
        send(clientSocket, buf, bytesRecieved + 1, 0);
    }
}

void NetworkManager::Update(std::function<void(const std::string&)> messageCallback) {
    if (m_ListeningSocket == INVALID_SOCKET) return;

    HandleNewConnections();
    PollClientData(messageCallback);
}

void NetworkManager::HandleNewConnections() {
    sockaddr_in client;
    int clientSize = sizeof(client);

    // Because we set non-blocking mode, accept() will instantly return INVALID_SOCKET 
    // with an error of WSAEWOULDBLOCK if no one is trying to connect right now.
    SOCKET clientSocket = accept(m_ListeningSocket, (sockaddr*)&client, &clientSize);

    if (clientSocket != INVALID_SOCKET) {
        // Set the new client socket to non-blocking too so recv() doesn't freeze us
        u_long mode = 1;
        ioctlsocket(clientSocket, FIONBIO, &mode);

        m_ClientSockets.push_back(clientSocket);
        std::cout << "[NET] New player connected! Total players: " << m_ClientSockets.size() << "\n";
    }
}

void NetworkManager::PollClientData(std::function<void(const std::string&)> messageCallback) {
    char buf[4096];

    // Iterate backwards through the clients so we can safely erase disconnected ones
    for (int i = m_ClientSockets.size() - 1; i >= 0; --i) {
        SOCKET sock = m_ClientSockets[i];
        ZeroMemory(buf, 4096);

        int bytesReceived = recv(sock, buf, 4096, 0);

        if (bytesReceived > 0) {
            // Success! Convert raw bytes to a string packet and send it up to Application
            std::string msg(buf, bytesReceived);
            messageCallback(msg);
        }
        else if (bytesReceived == 0 || (bytesReceived == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)) {
            // Client disconnected or dropped out due to a real error
            std::cout << "[NET] Player disconnected.\n";
            closesocket(sock);
            m_ClientSockets.erase(m_ClientSockets.begin() + i);
        }
    }
}
void NetworkManager::SendToClient(SOCKET clientSocket, const std::string& message) {
    send(clientSocket, message.c_str(), static_cast<int>(message.length()), 0);
}

void NetworkManager::Shutdown() {
    for (SOCKET sock : m_ClientSockets) {
        closesocket(sock);
    }
    m_ClientSockets.clear();

    if (m_ListeningSocket != INVALID_SOCKET) {
        closesocket(m_ListeningSocket);
        m_ListeningSocket = INVALID_SOCKET;
    }
}
