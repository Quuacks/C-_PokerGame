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
            std::function<void(SOCKET)> disconnectCallback) {

    FD_ZERO(&m_ReadSet);

    FD_SET(m_ListeningSocket, &m_ReadSet);

    for (SOCKET rawSock : m_RawSockets) {
        FD_SET(rawSock, &m_ReadSet);
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
        HandleNewConnections();

        PollRawSockets(rawCallback);
    }
}

void NetworkManager::HandleNewConnections() {
    if (FD_ISSET(m_ListeningSocket, &m_ReadSet)) {
        sockaddr_in client;
        int clientSize = sizeof(client);

        SOCKET clientSocket = accept(m_ListeningSocket, (sockaddr*)&client, &clientSize);

        if (clientSocket != INVALID_SOCKET) {
            u_long mode = 1;
            ioctlsocket(clientSocket, FIONBIO, &mode); // Set to non-blocking

            m_RawSockets.push_back(clientSocket);
            std::cout << "[Net] Raw Connection established on socket " << clientSocket << ", waiting for Login...\n";
        }
    }
}
void NetworkManager::PollRawSockets(std::function<void(SOCKET, const std::string&)> rawCallback) {
    for (auto it = m_RawSockets.begin(); it != m_RawSockets.end(); ) {
        SOCKET rawSock = *it;

        if (FD_ISSET(rawSock, &m_ReadSet)) {
            char buffer[4096];
            int bytesReceived = recv(rawSock, buffer, sizeof(buffer) - 1, 0);

            if (bytesReceived == 0) {
                std::cout << "[NETWORK] Unauthenticated raw client disconnected gracefully on socket: " << rawSock << "\n";
                closesocket(rawSock);
                it = m_RawSockets.erase(it);
                continue;
            }
            else if (bytesReceived == SOCKET_ERROR) {
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) {
                    ++it;
                    continue;
                }
                std::cout << "[NETWORK] Unauthenticated raw client connection lost forcibly on socket: " << rawSock << "\n";
                closesocket(rawSock);
                it = m_RawSockets.erase(it);
                continue;
            }
            else {
                buffer[bytesReceived] = '\0';
                std::string message(buffer);

                rawCallback(rawSock, message);

                it = m_RawSockets.begin();
                continue;
            }
        }
        ++it;
    }
}

void NetworkManager::MoveRawSocketToPlayer(SOCKET socket) {
    auto it = std::find(m_RawSockets.begin(), m_RawSockets.end(), socket);

    if (it != m_RawSockets.end()) {
        m_RawSockets.erase(it);
        std::cout << "[NETWORK] Cleanly removed socket " << socket << " from raw connection pool.\n";
    }
    else {
        // If it gets here, it means PollRawSockets already handled the erasure, which is perfect.
        std::cout << "[NETWORK] Socket " << socket << " was already cleared from raw pool. Skipping safely.\n";
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

    WSACleanup();
}
