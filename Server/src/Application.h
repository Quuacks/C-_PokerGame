#pragma once
#include <string>
#include <chrono>
#include <thread>
#include "networkManager.h"
#include "game/roomManager.h"
#include "core/logging.h"

class Application
{
public:
    explicit Application() 
    {
        LOG_TRACE("Log test {}", "a");
        MainLoop();
    }
private:
    void MainLoop() 
    {
        while (true) 
        {
            m_NetworkManager.Update([&](const std::string& msg) { OnReceiveMessage(msg); });
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
private:
    void OnReceiveMessage(const std::string& message) 
    {

    }
private:
    RoomManager m_RoomManager;
    NetworkManager m_NetworkManager;
};

