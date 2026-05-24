#include "application.h"
#include <WS2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

Application::Application()
{
    std::cout << "Application initialized.\n";
}

void Application::RegisterHandlers() 
{
    m_RawHandlers["LOGIN"] = [this](SOCKET sock, const json& data) { HandleLogin(sock, data); };

    // Register Player handlers (authenticated users)
    m_PlayerHandlers["RAISE"] = [this](Player& p, const json& data) { HandleRaise(p, data); };
    m_PlayerHandlers["FOLD"] = [this](Player& p, const json& data) { HandleFold(p, data); };
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
    
    json packet = json::parse(message);
    std::string type = packet.value("type", "");
    json data = packet.value("data", json::object());

    auto it = m_Handlers.find(type);

    // 2. Make ABSOLUTELY sure the right side matches 'm_Handlers' exactly
    if (it != m_Handlers.end()) {
        // 3. Because it's a unique_ptr, execute via the arrow operator
        //it->second->Execute(rawSocket, data);
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
void Application::HandleLogin(SOCKET rawSocket, const nlohmann::json& data) {
    // 1. Extract the username from the JSON payload safely
    // If the "username" key is missing, it defaults to "Unknown_Pro"
    std::string username = data.value("username", "Unknown_Pro");

    std::cout << "[SERVER] Processing login request for user: " << username << "\n";


    std::cout << "[SERVER] User '" << username << "' logged in successfully.\n";
}

void Application::HandleRaise(Player& player, const nlohmann::json& data) {
    // 1. Extract the raise amount out of the nested JSON packet
    int raiseAmount = data.value("amount", 0);

    // 2. Fetch the player's name (assuming your Player class has GetName())
    std::cout << "[SERVER] Player raised by: " << raiseAmount << " chips.\n";

    
}

void Application::HandleFold(Player& player, const nlohmann::json& data) {
    std::cout << "[SERVER] Player folded their hand.\n";

    
}

