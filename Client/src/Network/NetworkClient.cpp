#include "NetworkClient.h"
#include <iostream>

NetworkClient::NetworkClient()
    : m_Socket(INVALID_SOCKET), m_isConnected(false)
{}

NetworkClient::~NetworkClient()
{
    Disconnect();
}

bool NetworkClient::ConnectToServer(const std::string& ipAddress, int port)
{
    m_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_Socket == INVALID_SOCKET)
    {
        std::cerr << "[Client] Failed to create socket\n";
        return false;
    }

    sockaddr_in hint{};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

    std::cout << "[CLIENT] Connecting to server at " << ipAddress << ":" << port << "...\n";

    if (connect(m_Socket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR)
    {
        std::cerr << "[Client] Connection failed. " << WSAGetLastError() << "\n";
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
        return false;
    }

    // Non-blocking mode
    u_long mode = 1;
    ioctlsocket(m_Socket, FIONBIO, &mode);

    m_isConnected = true;
    std::cout << "[Client] Connected to server\n";
    return true;
}

void NetworkClient::Update(std::function<void(const std::string&)> messageCallback)
{
    if (!m_isConnected || m_Socket == INVALID_SOCKET)
        return;

    char buf[4096];
    int bytes = recv(m_Socket, buf, sizeof(buf), 0);

    if (bytes > 0)
    {
        m_recvBuffer.append(buf, bytes);

        // Process all complete JSON packets (ending with '\n')
        size_t pos;
        while ((pos = m_recvBuffer.find('\n')) != std::string::npos)
        {
            std::string msg = m_recvBuffer.substr(0, pos);
            m_recvBuffer.erase(0, pos + 1);

            messageCallback(msg);
        }
    }
    else if (bytes == 0)
    {
        std::cout << "[Client] Server closed connection\n";
        Disconnect();
    }
    else
    {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK)
        {
            std::cout << "[Client] recv error: " << err << "\n";
            Disconnect();
        }
    }
}

bool NetworkClient::SendRequest(const std::string& message)
{
    if (!m_isConnected || m_Socket == INVALID_SOCKET)
        return false;

    int result = send(m_Socket, message.c_str(), (int)message.length(), 0);

    if (result == SOCKET_ERROR)
    {
        std::cerr << "[Client] Send failed. " << WSAGetLastError() << "\n";
        Disconnect();
        return false;
    }

    return true;
}

void NetworkClient::Disconnect()
{
    if (m_Socket != INVALID_SOCKET)
    {
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
    }

    m_isConnected = false;
}
