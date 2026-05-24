#include "application.h"
#include <WS2tcpip.h>
#include <iostream>

Application::Application()
{
    std::cout << "Application initialized.\n";
}

void Application::RegisterHandlers() 
{
    std::cout << "Message handlers registered";
}

void Application::Start() {
    std::cout << "Starting server...\n";
    if (!m_NetworkManager.Initialize(54000))
    {
        std::cout << "Failed to initialize Network Manager." << std::endl;
        return;
    }

    std::cout << "Server successfully listening on port 54000..." << std::endl;

    MainLoop();
}

void Application::MainLoop()
{

    while (true) {
        m_NetworkManager.Update([&](const std::string& msg) {
            OnReceiveMessage(msg);
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

}

void Application::OnReceiveMessage(const std::string& message)
{
    size_t delimiterPos = message.find('|');
    if (delimiterPos == std::string::npos)
    {
        std::cout << "[WARNING] Received malformed message: " << message << "\n";
        return;
    }

    // Split the message into type and data payload
    std::string messageType = message.substr(0, delimiterPos);
    std::string payload = message.substr(delimiterPos + 1);

    std::cout << "[ROUTER] Type: " << messageType << " | Payload: " << payload << "\n";

    //// Route to the appropriate handler
    //auto it = m_Handlers.find(messageType);
    //if (it != m_Handlers.end())
    //{
    //    // Execute the handler logic
    //    // Note: Later you'll pass a Player session pointer here instead of nullptr
    //    it->second->handleRequest(nullptr, payload);
    //}
    //else
    //{
    //    std::cout << "[WARNING] No handler registered for message type: " << messageType << "\n";
    //}
}
