#include "GameState.h"
#include "UI/UI.h"
#include <json.hpp>

using json = nlohmann::json;

GameState ParseGameState(const json& data)
{
    GameState state;

    state.pot = data.value("pot", 0);
    state.selectedComboIndex = data.value("selectedComboIndex", 0);
    state.heroChips = data.value("heroChips", 1500);

    if (data.contains("isHeroTurn") && data["isHeroTurn"].is_boolean())
        state.isHeroTurn = data["isHeroTurn"].get<bool>();

    if (data.contains("isYourTurn") && data["isYourTurn"].is_boolean())
        state.isHeroTurn = data["isYourTurn"].get<bool>();

    if (data.contains("yourTurn") && data["yourTurn"].is_boolean())
        state.isHeroTurn = data["yourTurn"].get<bool>();

    if (data.contains("players") && data["players"].is_array())
    {
        for (auto& p : data["players"])
            state.players.push_back(Utf8ToWide(p.get<std::string>()));
    }

    if (data.contains("boardCards") && data["boardCards"].is_array())
    {
        for (auto& c : data["boardCards"])
            state.boardCards.push_back({
                c.value("rank", 0),
                c.value("suit", 0)
                });
    }

    if (data.contains("heroCards") && data["heroCards"].is_array())
    {
        for (auto& c : data["heroCards"])
            state.heroCards.push_back({
                c.value("rank", 0),
                c.value("suit", 0)
                });
    }

    if (data.contains("playerChips") && data["playerChips"].is_array())
    {
        for (auto& chips : data["playerChips"])
            state.playerChips.push_back(chips.get<int>());
    }

    return state;
}
