#include "networkManager.h"
#include <WS2tcpip.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

NetworkManager::NetworkManager()
    : m_ListeningSocket(INVALID_SOCKET) {
}

NetworkManager::~NetworkManager() {
    Shutdown();
}

bool NetworkManager::Initialize(int port)
{
    m_ListeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_ListeningSocket == INVALID_SOCKET) {
        std::cout << "Cant create a socket (error " << WSAGetLastError() << ")\n";
        return false;
    }

    sockaddr_in hint{};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(static_cast<u_short>(port));
    hint.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_ListeningSocket, reinterpret_cast<sockaddr*>(&hint), sizeof(hint)) == SOCKET_ERROR) {
        std::cout << "Cant bind socket (error " << WSAGetLastError() << ")\n";
        closesocket(m_ListeningSocket);
        m_ListeningSocket = INVALID_SOCKET;
        return false;
    }
    std::cout << "Socket bound successfully on port " << port << "\n";

    if (listen(m_ListeningSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Cant listen on socket (error " << WSAGetLastError() << ")\n";
        closesocket(m_ListeningSocket);
        m_ListeningSocket = INVALID_SOCKET;
        return false;
    }
    std::cout << "Socket listening successfully\n";

    u_long nonBlocking = 1;
    if (ioctlsocket(m_ListeningSocket, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        std::cout << "Cant set non-blocking mode (error " << WSAGetLastError() << ")\n";
        closesocket(m_ListeningSocket);
        m_ListeningSocket = INVALID_SOCKET;
        return false;
    }

    return true;
}

void NetworkManager::Update(std::function<void(const std::string&)> messageCallback) {
    if (m_ListeningSocket == INVALID_SOCKET) return;

    HandleNewConnections();
    PollClientData(messageCallback);
}

void NetworkManager::HandleNewConnections() {
    sockaddr_in client;
    int clientSize = sizeof(client);

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
            //If there was something sent it sends the message to application.cpp with messageCalback
            //wich is a void pointer to a function in application.cpp file
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
