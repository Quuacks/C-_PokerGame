#pragma once
#include <optional>
#include <core/result.h>
#include "player.h"
#include <vector>
class Room
{
public:
    Room(Player& adminPlayer);
    std::optional<ResultError> TryStartGame(std::string performedByUsername);
    void AddPlayer(Player player);
private:
    Player& m_AdminPlayer;
    std::vector<Player> m_Players;
};
