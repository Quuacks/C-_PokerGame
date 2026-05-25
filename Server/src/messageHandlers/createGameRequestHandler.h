#pragma once
#include <handler/requestHandler.h>
#include "../Application.h"

class CreateGameRequestHandler : public requestHandler
{ 
    void ExecuteRaw(Application& app, SOCKET rawSocket, const nlohmann::json& data) override
    {

    }
};

