#include "application.h"
#include <WS2tcpip.h>
#include <iostream>

Application::Application()
{
    LOG_TRACE("Log test {}", "a");
}

void Application::Start() {
    if (!m_NetworkManager.Initialize(54000))
    {
        std::cout << "Failed to initialize Network Manager." << std::endl;
        return;
    }

    std::cout << "Server successfully listening on port 54000..." << std::endl;

    MainLoop();
}

inline void Application::MainLoop()
{

    while (true) {
        m_NetworkManager.Update([&](const std::string& msg) {
            OnReceiveMessage(msg);
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

}

inline void Application::OnReceiveMessage(const std::string& message)
{

}
