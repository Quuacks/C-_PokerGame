#pragma once
#include <handler/requestHandler.h>
#include "../game/player.h"
#include "../Application.h"
#include <iostream>

class playerActionRequestHandler : public requestHandler
{
    public:
        void ExecutePlayer(Application& app, Player& player, const nlohmann::json& data) override {
            std::cout << "Execute player actions  PLAYER: " << player.GetUsername() << "\n";
        }
};
