#include "application.h"
#include <WS2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>
#include "messageHandlers/joinGameRequestHandler.h"
#include "messageHandlers/playerActionRequestHandler.h"

Application::Application()
{
    std::cout << "Application initialized.\n";
}

void Application::RegisterHandlers() 
{
    m_Handlers["LOGIN"] = std::make_unique<joinGameRequestHandler>();
    m_Handlers["RAISE"] = std::make_unique<playerActionRequestHandler>();
    m_Handlers["FOLD"] = std::make_unique<playerActionRequestHandler>();
    m_Handlers["CHECK"] = std::make_unique<playerActionRequestHandler>();
}

void Application::Start() {
    std::cout << "Starting server...\n";
    if (!m_NetworkManager.Initialize(54000))
    {
        std::cout << "Failed to initialize Network Manager." << std::endl;
        return;
    }

    std::cout << "Server successfully listening on port 54000..." << std::endl;
    RegisterHandlers();
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
    
    json packet = json::parse(message);
    std::string type = packet.value("type", "");
    json data = packet.value("data", json::object());

    auto it = m_Handlers.find(type);

    // 2. Make ABSOLUTELY sure the right side matches 'm_Handlers' exactly
    if (it != m_Handlers.end()) {

        it->second->ExecuteRaw(*this, rawSocket, data);
    }
    else {
        std::cout << "[SERVER] Unknown action type: " << type << "\n";
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

void Application::AddAuthenticatedPlayer(SOCKET socket, const std::string& username) {
    Player newPlayer(socket, username);

    m_Players.push_back(newPlayer);

    std::cout << "[Server] " << username << "has joined the server. Total players: " << m_Players.size() << "\n";
    //send login confirm json packet to client
}

