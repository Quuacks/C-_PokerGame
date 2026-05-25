#pragma once
#include <json.hpp>
#include "GameState.h"

GameState ParseGameState(const nlohmann::json& data);
