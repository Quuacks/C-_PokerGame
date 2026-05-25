#pragma once
#include "RequestHandler.h"
#include "../Application.h"
#include <iostream>

class joinGameRequestHandler : public requestHandler
{
public:
    void ExecuteRaw(Application& app, SOCKET rawSocket, const nlohmann::json& data) override {
        std::string username = data.value("username", "Guest");

        app.AddAuthenticatedPlayer(rawSocket, username);
    }
};
