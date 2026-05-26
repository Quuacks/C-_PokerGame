#pragma once
#include <handler/requestHandler.h>
#include "../game/player.h"
#include "../Application.h"
#include <iostream>

class playerActionRequestHandler : public requestHandler
{
    public:
        void ExecutePlayer(Application& app, Player& player, const nlohmann::json& data) override;

        void ExecuteRaw(Application& app, SOCKET rawSocket, const nlohmann::json& data) override {
            std::string actionType = data.value("type", "");
            int amount = data.value("amount", 0);

            std::cout << "Execute player actions  PLAYER: " << rawSocket << "\n" << "ACTION: " << actionType << "\n";

            auto& table = app.GetGameTable();
            table.ProcessPlayerAction(rawSocket, actionType, amount);

            app.BroadcastGameState();
        }
};
