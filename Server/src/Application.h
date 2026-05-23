#pragma once
#include <string>
#include <chrono>
#include <thread>
#include "Network/networkManager.h"
#include "game/roomManager.h"
#include "core/logging.h"
#include <WS2tcpip.h>
#include <iostream>

class Application
{
public:
    explicit Application();
    void Start();
private:
    void MainLoop();
private:
    void OnReceiveMessage(const std::string& message);
private:
    RoomManager m_RoomManager;
    NetworkManager m_NetworkManager;
};

