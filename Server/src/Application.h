#pragma once
#include <string>
#include <chrono>
#include <thread>
#include "NetworkManager.h"

class Application
{
public:
    explicit Application() 
    {

    }
private:
    void Update() 
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
    NetworkManager m_NetworkManager;
};

