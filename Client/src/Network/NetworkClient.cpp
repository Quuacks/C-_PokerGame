#include "NetworkClient.h"
#include <iostream>

NetworkClient::NetworkClient() 
    : m_Socket(INVALID_SOCKET), m_isConnected(false){}

NetworkClient::~NetworkClient() {
    Disconnect();
}

bool NetworkClient::ConnectToServer(const std::string& ipAddress, int port) {
    m_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_Socket == INVALID_SOCKET) {
        std::cerr << "[Client] failed to create socket\n";
        return false;
    }

    sockaddr_in hint{};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

    std::cout << "[CLIENT] Connecting to server at " << ipAddress << ":" << port << "...\n";
    int connResult = connect(m_Socket, (sockaddr*)&hint, sizeof(hint));
    if (connResult == SOCKET_ERROR) {
        std::cerr << "[Client] Connection failed. " << WSAGetLastError() << "\n";
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
        return false;
    }

    u_long mode = 1;
    ioctlsocket(m_Socket, FIONBIO, &mode);

    m_isConnected = true;
    std::cout << "[Client] connected to server\n";
    return true;
}

void NetworkClient::Update(std::function<void(const std::string&)> messageCallback) {
    if (!m_isConnected || m_isConnected == INVALID_SOCKET)
        return;

    char buf[4096];
    ZeroMemory(buf, 4096);

    int bytesRecevied = recv(m_Socket, buf, 4096, 0);

    if (bytesRecevied > 0) {
        std::string serverMessage(buf, bytesRecevied);
        messageCallback(serverMessage);
    }
    else if (bytesRecevied == 0) {
        std::cout << "[Client] Server shut down connection\n";
        Disconnect();
    }
    else {
        int errorCode = WSAGetLastError();

        if (errorCode != WSAEWOULDBLOCK) {
            std::cout << "[Client] we done fucked up " << errorCode << std::endl;
            Disconnect();
        }
    }
}

bool NetworkClient::SendRequest(const std::string& message) {
    if (!m_isConnected || m_Socket == INVALID_SOCKET)
        return false;

    int sendResult = send(m_Socket, message.c_str(), static_cast<int>(message.length()), 0);
    if (sendResult == SOCKET_ERROR) {
        std::cerr << "[Client] Send failed. " << WSAGetLastError() << "\n";
        Disconnect();
        return false;
    }
}

void NetworkClient::Disconnect() {
    if (m_Socket != INVALID_SOCKET) {
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
    }
    m_isConnected = false;
}
