#pragma once
#include <string>
#include "NetworkManager.h"

class Application
{
public:
    explicit Application() {

    }

private:
    void AcceptIncomingConnections() {

    }

    std::string TryAcceptMessages() {

    }

    void SendPendingMessages() {

    }

    void Update() {
        m_NetworkManager.Update([&](const std::string& msg) { OnReceiveMessage(msg); });
    }

private:
    void OnReceiveMessage(const std::string& message) {

    }
private:
    NetworkManager m_NetworkManager;
};

