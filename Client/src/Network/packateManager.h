#pragma once
#include <handler/requestHandler.h>
#include <string>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

class packateManager
{
    public:
        std::unordered_map<std::string, std::function<void(const nlohmann::json&)>> m_ServerHandlers;
};

