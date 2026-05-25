#pragma once
#include <handler/requestHandler.h>
#include "../game/player.h"
#include "../Application.h"
#include <iostream>

class playerActionRequestHandler : public requestHandler
{
    public:
        void ExecutePlayer(Application& app, Player& player, const nlohmann::json& data) override {
            std::string actionType = data.value("type", "");
            int amount = data.value("amount", 0);
            
            std::cout << "Execute player actions  PLAYER: " << player.GetUsername() << "\n" << "ACTION: " << actionType << "\n";

            auto& table = app.GetGameTable();
            table.ProcessPlayerAction(player.getSocket(), actionType, amount);

            app.BroadcastGameState();
        }
};
