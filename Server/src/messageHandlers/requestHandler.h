#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <winsock2.h>

class Application;
class Player;

class requestHandler
{ 
    public:
        virtual ~requestHandler() = default;

        virtual void ExecuteRaw(Application& app, SOCKET rawSocket, const nlohmann::json& data){}

        virtual void ExecutePlayer(Application& app, Player& player, const nlohmann::json& data){}
};

