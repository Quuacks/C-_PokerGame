#include "application.h"
#include <WS2tcpip.h>
#include <iostream>
#include <nlohmann/json.hpp>

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
        m_NetworkManager.Update(
            m_Players,
            [&](SOCKET sock, const std::string& msg) { HandleRawMessage(sock, msg); },
            [&](Player& player, const std::string& msg) { HandlePlayerMessage(player, msg); }
        );

        //main game loop

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

}
void Application::HandleRawMessage(SOCKET rawSocket, const std::string& message) {
    size_t delimiterPos = message.find('|');
    if (delimiterPos == std::string::npos) return;

    std::string type = message.substr(0, delimiterPos);
    std::string payload = message.substr(delimiterPos + 1);

    if (type == "LOGIN") {
        Player newPlayer(rawSocket, payload);
        m_Players.push_back(newPlayer);

        m_NetworkManager.MoveRawSocketToPlayer(rawSocket);

        std::cout << "[APP] " << payload << " authenticated. Total active players: " << m_Players.size() << "\n";
        m_NetworkManager.SendToClient(rawSocket, "LOGIN_SUCCESS|Welcome!\n");
    }
}

void Application::HandlePlayerMessage(Player& player, const std::string& message) {
    size_t delimiterPos = message.find('|');

    if (delimiterPos == std::string::npos)
        return;

    std::string type = message.substr(0, delimiterPos);
    std::string payload = message.substr(delimiterPos + 1);

    //game logic parsing here. Things like Player Actions
    std::cout << "TYPE: " << type << "   PAYLOAD: " << payload << "\n";
}


