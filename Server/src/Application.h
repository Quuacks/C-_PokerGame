#pragma once
#include <string>
#include <chrono>
#include <thread>
#include "Network/networkManager.h"
#include "game/roomManager.h"
#include "core/logging.h"
#include <WS2tcpip.h>
#include <iostream>
#include <messageHandlers/requestHandler.h>

class Application
{
public:
    Application();
    void Start();
private:
    void MainLoop();
    void OnReceiveMessage(const std::string& message);
    void RegisterHandlers();

    RoomManager m_RoomManager;
    NetworkManager m_NetworkManager;

    std::map<std::string, std::unique_ptr<RequestHandler>> m_Handlers;
};

