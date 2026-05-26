#include "application.h"
#include <WS2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <chrono>
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
    m_Handlers["CALL"] = std::make_unique<playerActionRequestHandler>();
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
    // Initialize so first meaningful state change broadcasts immediately.
    auto lastBroadcast = std::chrono::steady_clock::now() - std::chrono::seconds(2);
    int lastPot = -1;
    size_t lastBoardCount = 0;
    SOCKET lastTurnSocket = INVALID_SOCKET;
    auto lastState = m_Table.GetTableState();

    while (true) {
        m_NetworkManager.Update(
            m_Players,
            [&](SOCKET sock, const std::string& msg) { HandleRawMessage(sock, msg); },
            [&](Player& player, const std::string& msg) { HandlePlayerMessage(player, msg); },
            [&](SOCKET disconnectedSocket) { m_Table.RemovePlayer(disconnectedSocket); }
        );

        m_Table.Tick();

        // Broadcast only when the table state actually changes (and not while waiting for players).
        const bool enoughPlayers = m_Table.GetPlayerCount() >= 2;

        const int pot = m_Table.getPot();
        const size_t boardCount = m_Table.getCommunityCards().size();
        const SOCKET turnSocket = m_Table.GetCurrentTurnSocket();
        const auto tableState = m_Table.GetTableState();

        auto now = std::chrono::steady_clock::now();
        const bool minCadencePassed = (now - lastBroadcast >= std::chrono::milliseconds(500));
        const bool stateChanged =
            pot != lastPot ||
            boardCount != lastBoardCount ||
            turnSocket != lastTurnSocket ||
            tableState != lastState;

        if (enoughPlayers && minCadencePassed && stateChanged) {
            BroadcastGameState();
            lastBroadcast = now;
            lastPot = pot;
            lastBoardCount = boardCount;
            lastTurnSocket = turnSocket;
            lastState = tableState;
        }

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
    std::cout << "[NETWORK INBOUND] Raw string received from " << player.GetUsername() << ": " << message << std::endl;

    if (message.empty())
        return;
    
    try {
        json packet = json::parse(message);
        std::string type = packet.value("type", "");

        auto it = m_Handlers.find(type);
        if (it != m_Handlers.end()) {
            // Packet shape from client: { "type": "...", "data": { ... } }
            // Handlers expect the JSON "data" payload.
            std::cout << "[Server] Routing type '" << type << "' to message handler..." << std::endl;

            it->second->ExecutePlayer(*this, player, packet.value("data", json::object()));
            BroadcastGameState();
        }
        else {
            std::cout << "[Server] Uknown player action " << type << "\n";
        }

    }
    catch (const json::parse_error& e){
        std::cout << "[Server] JSON Parse Error from player " << player.GetUsername() << ": " << e.what() << "\n";
    }

    
}

void Application::AddAuthenticatedPlayer(SOCKET socket, const std::string& username) {
    auto newPlayer = std::make_shared<Player>(socket, username);

    m_Players.push_back(newPlayer);

    std::cout << "[Server] " << username << "has joined the server. Total players: " << m_Players.size() << "\n";
    
    m_Table.AddPlayer(newPlayer);

    //m_NetworkManager.MoveRawSocketToPlayer(socket);
}

void Application::BroadcastGameState()
{
    if (m_Players.empty())
        return;

    json packet;
    packet["type"] = "GAME_STATE"; // Matches client's routing router

    json data;
    data["pot"] = m_Table.getPot();
    data["selectedComboIndex"] = 0; // Fallback or computed combo value

    // Arrays for table layout
    json playersArray = json::array();
    json chipsArray = json::array();

    for (const auto& playerPtr : m_Players) {
        if (!playerPtr) continue;
        playersArray.push_back(playerPtr->GetUsername());
        chipsArray.push_back(playerPtr->getChips());
    }
    data["players"] = playersArray;
    data["playerChips"] = chipsArray;

    // Map community cards
    json boardCardsArray = json::array();
    for (const auto& card : m_Table.getCommunityCards()) {
        boardCardsArray.push_back({ {"rank", card.rank}, {"suit", card.suit} });
    }
    data["boardCards"] = boardCardsArray;

    // Loop through players individually to send private hole cards and hero metrics
    for (const auto& playerPtr : m_Players) {
        if (!playerPtr) continue;

        json privateData = data; // Clone master table data layout

        // Populate fields specific to this connection
        privateData["heroChips"] = playerPtr->getChips();
        bool isYourTurn = (m_Table.GetCurrentTurnSocket() == playerPtr->getSocket());
        // Client UI/parser currently expects `isHeroTurn`, but we keep `isYourTurn` as an alias.
        privateData["isHeroTurn"] = isYourTurn;
        privateData["isYourTurn"] = isYourTurn;

        json heroCardsArray = json::array();
        for (const auto& card : playerPtr->GetHoleCards()) {
            heroCardsArray.push_back({ {"rank", card.rank}, {"suit", card.suit} });
        }
        privateData["heroCards"] = heroCardsArray;

        // Wrap payload up
        packet["data"] = privateData;
        std::string payload = packet.dump() + "\n";

        // Send down the pipe to this specific client player
        send(playerPtr->getSocket(), payload.c_str(), static_cast<int>(payload.length()), 0);
    }
}
