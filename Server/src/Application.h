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
#include <handler/requestHandler.h>
#include <nlohmann/json.hpp>
#include "game/gameState.h"
#include "game/player.h"

using json = nlohmann::json;

class Application
{
public:
    Application();
    void Start();
    void AddAuthenticatedPlayer(SOCKET socket, const std::string& username);
    void BroadcastGameState();
private:
    GameState m_Game;         
    void MainLoop();
    void HandleRawMessage(SOCKET rawSocket, const std::string& message);
    void HandlePlayerMessage(Player& player, const std::string& message);
    
    void RegisterHandlers();

    void HandleLogin(SOCKET rawSocket, const nlohmann::json& data);
    void HandleRaise(Player& player, const nlohmann::json& data);
    void HandleFold(Player& player, const nlohmann::json& data);

    RoomManager m_RoomManager;
    NetworkManager m_NetworkManager;

    std::unordered_map<std::string, std::unique_ptr<requestHandler>> m_Handlers;

    std::unordered_map<std::string, std::function<void(Application&, SOCKET, const json&)>> m_RawHandlers;

    // Player routes map a string type to a function that takes a Player reference and JSON data
    std::unordered_map<std::string, std::function<void(Player&, const json&)>> m_PlayerHandlers;

    std::vector<Player> m_Players;
};

