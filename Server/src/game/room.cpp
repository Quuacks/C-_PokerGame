#include "room.h"

Room::Room(Player& adminPlayer)
    : m_AdminPlayer(adminPlayer) {
}

std::optional<ResultError> Room::TryStartGame(std::string performedByUsername) {
    if (m_AdminPlayer.GetUsername() != performedByUsername) {
        return std::make_optional(ResultError("You must be an admin user to start the game!"));
    }
}

void Room::AddPlayer(Player player) {
    m_Players.push_back(std::move(player));
}
