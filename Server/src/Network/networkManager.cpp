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

void NetworkManager::Update(std::vector<std::shared_ptr<Player>>& authenticatedPlayers,
            std::function<void(SOCKET, const std::string&)> rawCallback,
            std::function<void(Player&, const std::string&)> playerCallback) {

    FD_ZERO(&m_ReadSet);

    FD_SET(m_ListeningSocket, &m_ReadSet);

    for (const auto& playerPtr : authenticatedPlayers) {
        if (playerPtr) {
            FD_SET(playerPtr->getSocket(), &m_ReadSet);
        }
    }

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    int activity = select(0, &m_ReadSet, nullptr, nullptr, &timeout);

    if (activity == SOCKET_ERROR) {
        std::cout << "[NETWORK ERROR] Select failed: " << WSAGetLastError() << "\n";
        return;
    }

    if (activity > 0) {
        PollRawSockets(rawCallback);

        PollAuthenticatedPlayers(authenticatedPlayers, playerCallback);
    }
}

void NetworkManager::HandleNewConnections() {
    sockaddr_in client;
    int clientSize = sizeof(client);

    SOCKET clientSocket = accept(m_ListeningSocket, (sockaddr*)&client, &clientSize);

    if (clientSocket != INVALID_SOCKET) {
        // Set the new client socket to non-blocking too so recv() doesn't freeze us
        u_long mode = 1;
        ioctlsocket(clientSocket, FIONBIO, &mode);

        m_RawSockets.push_back(clientSocket);
        std::cout << "[NET] Raw Connection established waiting for Login: \n";
    }
}
void NetworkManager::PollRawSockets(std::function<void(SOCKET, const std::string&)> rawCallback) {
    if (FD_ISSET(m_ListeningSocket, &m_ReadSet)) {
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);

        SOCKET newClientSocket = accept(m_ListeningSocket, (sockaddr*)&clientAddr, &addrLen);
        if (newClientSocket != INVALID_SOCKET) {
            std::cout << "[NETWORK] Raw client connection accepted on socket: " << newClientSocket << "\n";

            //Listen for Login payload
        }
    }
}

void NetworkManager::PollAuthenticatedPlayers(
    std::vector<std::shared_ptr<Player>>& players,
    std::function<void(Player&, const std::string&)> handler)
{
    for (auto it = players.begin(); it != players.end(); ) {
        auto& playerPtr = *it;
        SOCKET clientSocket = playerPtr->getSocket();

        // Use the passed-in readSet variable directly!
        if (FD_ISSET(clientSocket, &m_ReadSet)) {
            char buffer[4096];
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytesReceived <= 0) {
                std::cout << "[NETWORK] Player disconnected: " << playerPtr->GetUsername() << "\n";
                closesocket(clientSocket);
                it = players.erase(it);
                continue;
            }
            else {
                buffer[bytesReceived] = '\0';
                std::string message(buffer);
                handler(*playerPtr, message);
            }
        }
        ++it;
    }
}

void NetworkManager::MoveRawSocketToPlayer(SOCKET socket) {
    for (auto it = m_RawSockets.begin(); it != m_RawSockets.end(); ++it) {
        if (*it == socket) {
            m_RawSockets.erase(it);
            break;
        }
    }
}

void NetworkManager::SendToClient(SOCKET clientSocket, const std::string& message) {
    send(clientSocket, message.c_str(), static_cast<int>(message.length()), 0);
}

void NetworkManager::Shutdown() {
    for (SOCKET sock : m_RawSockets) {
        closesocket(sock);
    }
    m_RawSockets.clear();

    if (m_ListeningSocket != INVALID_SOCKET) {
        closesocket(m_ListeningSocket);
        m_ListeningSocket = INVALID_SOCKET;
    }
}
