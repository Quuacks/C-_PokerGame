#pragma once
#include <string>

constexpr int DEFAULT_PLAYER_CURRENCY = 500;

class Player
{
public:
    inline Player(std::string username)
        : m_Username(std::move(username)) { }

    inline const std::string& GetUsername() const {
        return m_Username;
    }
private:
    std::string m_Username;
    int m_Currency = DEFAULT_PLAYER_CURRENCY;
};
