#pragma once
#include <handler/requestHandler.h>
#include "../Application.h"
#include <iostream>

class joinGameRequestHandler : public requestHandler
{
public:
    void ExecuteRaw(Application& app, SOCKET rawSocket, const nlohmann::json& data) override {
        if (!data.contains("username")) {
            std::cout << "[ERROR] Login payload missing 'username' key.\n";
            return;
        }

        std::string username = data.value("username", "Guest");

        std::cout << "[Server] Player Loged In: " << username << "\n";

        app.AddAuthenticatedPlayer(rawSocket, username);
    }
};
