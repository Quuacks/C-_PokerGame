#pragma once
#include <string>
#include <WS2tcpip.h>

class Player
{
public:
    Player(SOCKET socket, std::string username)
        : m_Username(std::move(username)), m_Socket(socket) { }

    std::string GetUsername() const {
        return m_Username;
    }

    SOCKET getSocket() const {
        return m_Socket;
    }

    int getChips() {
        return m_Chips;
    }

    bool m_isFolded = false;

private:
    SOCKET m_Socket;
    int m_Chips;
    std::string m_Username;
};
