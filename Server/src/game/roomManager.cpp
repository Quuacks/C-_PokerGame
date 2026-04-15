#include "roomManager.h"

std::optional<ResultError> RoomManager::CreateRoom(const std::string& roomname, Player player)
{
    auto it = m_RoomNameToRoom.find(roomname);
    if (it != m_RoomNameToRoom.end())
    {
        return ResultError("Room already exists!");
    }
}

std::optional<ResultError> RoomManager::JoinRoom(const std::string& roomname, Player player) {
    auto it = m_RoomNameToRoom.find(roomname);
    if (it == m_RoomNameToRoom.end()) {
        return ResultError("Room does not exist!");
    }

    Room& room = (*it).second;
    room.AddPlayer(std::move(player));
}
