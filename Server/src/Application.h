#pragma once
#include <string>
#include <chrono>
#include <thread>
#include "Network/networkManager.h"
#include "game/roomManager.h"
#include "core/logging.h"
#include <WS2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <messageHandlers/requestHandler.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Application
{
public:
    Application();
    void Start();
private:
    void MainLoop();
    void HandleRawMessage(SOCKET rawSocket, const std::string& message);
    void HandlePlayerMessage(Player& player, const std::string& message);
    
    void RegisterHandlers();

    void HandleLogin(SOCKET rawSocket, const nlohmann::json& data);
    void HandleRaise(Player& player, const nlohmann::json& data);
    void HandleFold(Player& player, const nlohmann::json& data);

    RoomManager m_RoomManager;
    NetworkManager m_NetworkManager;

    std::map<std::string, std::unique_ptr<RequestHandler>> m_Handlers;

    std::unordered_map<std::string, std::function<void(SOCKET, const json&)>> m_RawHandlers;

    // Player routes map a string type to a function that takes a Player reference and JSON data
    std::unordered_map<std::string, std::function<void(Player&, const json&)>> m_PlayerHandlers;

    std::vector<Player> m_Players;
};

