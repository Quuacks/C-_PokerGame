#pragma once
#include <WS2tcpip.h>
#include <string>
#include <functional>
class NetworkClient
{
    public:
        NetworkClient();
        ~NetworkClient();

        bool ConnectToServer(const std::string& ipAddress, int port);

        void Update(std::function<void(const std::string&)> messaggeCallback);

        bool SendRequest(const std::string& msg);

        void Disconnect();

        SOCKET getSocket() const {
            return m_Socket; 
        }

    private:
        SOCKET m_Socket;
        bool m_isConnected;
};

