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

void NetworkManager::Update(std::vector<Player>& authenticatedPlayers,
            std::function<void(SOCKET, const std::string&)> rawCallback,
            std::function<void(Player&, const std::string&)> playerCallback) {

    if (m_ListeningSocket == INVALID_SOCKET) return;

    HandleNewConnections();
    PollRawSockets(rawCallback);
    PollAuthenticatedPlayers(authenticatedPlayers, playerCallback);
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
    char buf[4096];
    bool socketTransitioned = false;
    for (int i = m_RawSockets.size() - 1; i >= 0; --i) {
        SOCKET sock = m_RawSockets[i];
        if (sock == INVALID_SOCKET)
            continue;

        ZeroMemory(buf, 4096);
        int bytesReceived = recv(sock, buf, 4096, 0);

        if (bytesReceived > 0) {
            rawCallback(sock, std::string(buf, bytesReceived));
            socketTransitioned = true;
            break;
        }
        else if (bytesReceived == 0) {
            std::cout << "[NET] Unauthenticated Client closed connection.\n";
            closesocket(sock);
            m_RawSockets[i] = INVALID_SOCKET;
        }
        else {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                // Forceful window close (WSAECONNRESET)
                std::cout << "[NET] Unauthenticated Client forcefully disconnected (Error " << err << ").\n";
                closesocket(sock);
                m_RawSockets[i] = INVALID_SOCKET;
            }
        }
    }
}

void NetworkManager::PollAuthenticatedPlayers(std::vector<Player>& authenticatedPlayers,
    std::function<void(Player&, const std::string&)> playerCallback) {

    char buf[4096];
    for (int i = authenticatedPlayers.size() - 1; i >= 0; --i) {
        Player& player = authenticatedPlayers[i];
        SOCKET sock = player.getSocket();

        ZeroMemory(buf, 4096);
        int bytesReceived = recv(sock, buf, 4096, 0);

        if (bytesReceived > 0) {
            playerCallback(player, std::string(buf, bytesReceived));
        }
        else if (bytesReceived == 0 || (bytesReceived == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)) {
            std::cout << "[NET] Player '" << player.GetUsername() << "' disconnected.\n";
            closesocket(sock);
            authenticatedPlayers.erase(authenticatedPlayers.begin() + i); // Single source of truth removal!
        }
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
