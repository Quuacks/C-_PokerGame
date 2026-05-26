#include "playerActionRequestHandler.h"

void playerActionRequestHandler::ExecutePlayer(Application& app, Player& player, const nlohmann::json& data) {
    
    std::string actionType = data.value("type", "");
    int amount = data.value("amount", 0);

    std::cout << "Execute player actions  PLAYER: " << player.GetUsername() << "\n" << "ACTION: " << actionType << "\n";

    auto& table = app.GetGameTable();
    table.ProcessPlayerAction(player.getSocket(), actionType, amount);

    app.BroadcastGameState();
}
