#pragma once
#include <vector>
#include <string>

// Must match your server-side Card struct
struct Card
{
    int rank;   // 1–13, 0 = backside
    int suit;   // 0=♥, 1=♠, 2=♦, 3=♣
};

// Must match your UI expectations
struct GameState
{
    int pot = 0;

    std::vector<Card> boardCards;
    std::vector<Card> heroCards;

    std::vector<std::wstring> players;
    std::vector<int> playerChips;

    int heroChips = 1500;
    int selectedComboIndex = 0;

    bool isHeroTurn = false;
};
