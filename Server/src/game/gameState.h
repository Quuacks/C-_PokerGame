#pragma once
#include <vector>
#include <string>

struct Card {
    int rank = 0;
    int suit = 0;
};

struct GameState {
    int pot = 0;

    std::vector<Card> boardCards;
    std::vector<Card> heroCards;

    std::vector<std::string> playerNames;
    std::vector<int> playerChips;

    int heroChips = 1500;
    int selectedComboIndex = 0;
};
