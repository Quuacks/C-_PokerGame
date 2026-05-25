#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <winsock2.h>

class Player;

class RequestHandler
{ 
    public:
        virtual ~RequestHandler() = default;

        virtual void ExecuteRaw(SOCKET rawSocket, const json& data){}

        virtual void ExecutePlayer(Player& player, const json& data){}
};

