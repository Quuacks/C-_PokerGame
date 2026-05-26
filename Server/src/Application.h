#pragma once
#include <string>
#include <chrono>
#include <thread>
#include "Network/networkManager.h"
#include "core/logging.h"
#include <WS2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <handler/requestHandler.h>
#include <nlohmann/json.hpp>
#include <game/GameTable.h>
#include "game/gameState.h"
#include "game/player.h"

using json = nlohmann::json;

class Application
{
public:
    Application();
    void Start();
    void AddAuthenticatedPlayer(SOCKET socket, const std::string& username);

    GameTable& GetGameTable() {
        return m_Table;
    };
    void BroadcastGameState();

private:
    GameState m_Game;         
    void MainLoop();
    void HandleRawMessage(SOCKET rawSocket, const std::string& message);
    
    void RegisterHandlers();

    void HandleLogin(SOCKET rawSocket, const nlohmann::json& data);
    void HandleRaise(Player& player, const nlohmann::json& data);
    void HandleFold(Player& player, const nlohmann::json& data);

    NetworkManager m_NetworkManager;

    std::unordered_map<std::string, std::unique_ptr<requestHandler>> m_Handlers;

    std::vector <std::shared_ptr<Player>> m_Players;

    GameTable m_Table;
};

