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
        BroadcastGameState();

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
    auto newPlayer = std::make_shared<Player>(socket, username);

    m_Players.push_back(newPlayer);

    std::cout << "[Server] " << username << "has joined the server. Total players: " << m_Players.size() << "\n";
    
    m_Table.AddPlayer(newPlayer);

    //Check how many players are currently
}

void Application::BroadcastGameState()
{
    //json packet;
    //packet["type"] = "GAME_STATE";

    //// Build data object explicitly because Card is a custom struct
    //json data;
    //data["pot"] = m_Game.pot;

    //data["boardCards"] = json::array();
    //for (const auto& c : m_Game.boardCards) {
    //    data["boardCards"].push_back({ {"rank", c.rank}, {"suit", c.suit} });
    //}

    //data["heroCards"] = json::array();
    //for (const auto& c : m_Game.heroCards) {
    //    data["heroCards"].push_back({ {"rank", c.rank}, {"suit", c.suit} });
    //}

    //data["players"] = m_Game.playerNames;
    //data["playerChips"] = m_Game.playerChips;
    //data["heroChips"] = m_Game.heroChips;
    //data["selectedComboIndex"] = m_Game.selectedComboIndex;

    //packet["data"] = data;

    //std::string msg = packet.dump() + "\n";

    //for (auto& p : m_Players)
    //    send(p->getSocket(), msg.c_str(), static_cast<int>(msg.size()), 0);
}
