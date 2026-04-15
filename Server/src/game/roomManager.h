#pragma once
#include <map>
#include <string>
#include <vector>
#include <optional>
#include "core/result.h"
#include "player.h"
#include "room.h"

class RoomManager
{
public:
    RoomManager() = default;
    std::optional<ResultError> CreateRoom(const std::string& roomname, Player player);
    std::optional<ResultError> JoinRoom(const std::string& roomname, Player player);
private:
    std::map<std::string, Room> m_RoomNameToRoom;
};
