#include "GameTable.h"
#include "player.h"
#include <iostream>
#include <random>
#include <chrono>

void GameTable::AddPlayer(std::shared_ptr<Player> player) {
    m_Players.push_back(player);
    std::cout << "[Table] Player " << player->GetUsername() << " joined the table\n";
}

void GameTable::RemovePlayer(SOCKET socket)
{
    m_Players.erase(std::remove_if(m_Players.begin(), m_Players.end(),
        [socket](const std::shared_ptr<Player>& p) { return p->getSocket() == socket; }), m_Players.end());
    std::cout << "[Table] Socket " << socket << " left the table\n";
}

void GameTable::StartNewHand()
{
    std::cout << "[Table] Starting new hand\n";
    m_Pot = 0;
    m_CurrentBet = 0;
    m_CurrentTurnIdx = 0;
    m_Round = BettingRound::PRE_FLOP;

    m_CommunityCards.clear();

    BuildDeck();
    ShuffleDeck();
    CollectBlinds();
    DealCards();
}

void GameTable::AdvanceBettingRound()
{
    if (m_Round == BettingRound::PRE_FLOP) {
        m_Round = BettingRound::FLOP;
        std::cout << "Moving To Flop";
        
        for (int i = 0; i < 3; ++i) {
            m_CommunityCards.push_back(m_Deck.back());
            m_Deck.pop_back();
        }
    }

}

void GameTable::ProcessPlayerAction(SOCKET socket, const std::string& actionType, int amount)
{
    auto activePlayer = m_Players[m_CurrentTurnIdx];
    if (activePlayer->getSocket() != socket) {
        std::cout << "[Warning] Player tried to act out of turn";
        return;
    }

    if (actionType == "FOLD") {
        if (actionType == "FOLD") {
            std::cout << "[ACTION] " << activePlayer->GetUsername() << " folds.\n";
            // TODO: Update player status to FOLDED inside your player class if tracking status
        }
        else if (actionType == "CALL") {
            int callAmount = m_CurrentBet; // Total needed to match the round bet
            m_Pot += callAmount;
            // activePlayer->DeductChips(callAmount);
            std::cout << "[ACTION] " << activePlayer->GetUsername() << " calls " << callAmount << "\n";
        }
        else if (actionType == "CHECK") {
            std::cout << "[ACTION] " << activePlayer->GetUsername() << " checks.\n";
        }
        else if (actionType == "RAISE") {
            m_CurrentBet = amount;
            m_Pot += amount;
            // activePlayer->DeductChips(amount);
            std::cout << "[ACTION] " << activePlayer->GetUsername() << " raises to " << amount << "\n";
        }

        if (IsBettingRoundComplete()) {
            AdvanceBettingRound();
        }
        else {
            MoveTurnToNextActivePlayer();
        }
    }
}
