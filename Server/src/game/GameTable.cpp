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

    for (auto& player : m_Players) {
        if (player) player->ClearHand();
    }

    BuildDeck();
    ShuffleDeck();
    CollectBlinds();
    DealCards();
}

void GameTable::AdvanceBettingRound()
{
    m_CurrentBet = 0;

    if (m_Round == BettingRound::PRE_FLOP) {
        m_Round = BettingRound::FLOP;
        std::cout << "Moving To Flop";
        
        for (int i = 0; i < 3; ++i) {
            m_CommunityCards.push_back(m_Deck.back());
            m_Deck.pop_back();
        }
    }
    else if (m_Round == BettingRound::FLOP) {
        m_Round = BettingRound::TURN;
        std::cout << "Moving To Turn\n";
        m_CommunityCards.push_back(m_Deck.back());
        m_Deck.pop_back();
    }
    else if (m_Round == BettingRound::TURN) {
        m_Round = BettingRound::RIVER;
        std::cout << "Moving to river";
        m_CommunityCards.push_back(m_Deck.back());
        m_Deck.pop_back();
    }
    else if (m_Round == BettingRound::RIVER) {
        m_Round = BettingRound::SHOWDOWN;
        std::cout << "Moving to Showdown";
        EvaluateShowdown();
        StartNewHand();
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
            std::cout << "[ACTION] " << activePlayer->GetUsername() << " folds.\n";
            // TODO: Update player status to FOLDED inside your player class if tracking status
        }
        else if (actionType == "CALL") {
            int callAmount = m_CurrentBet; // Total needed to match the round bet
            m_Pot += callAmount;
            activePlayer->RemoveChips(callAmount);
            std::cout << "[ACTION] " << activePlayer->GetUsername() << " calls " << callAmount << "\n";
        }
        else if (actionType == "CHECK") {
            std::cout << "[ACTION] " << activePlayer->GetUsername() << " checks.\n";
        }
        else if (actionType == "RAISE") {
            m_CurrentBet = amount;
            m_Pot += amount;
            activePlayer->RemoveChips(amount);
            std::cout << "[ACTION] " << activePlayer->GetUsername() << " raises to " << amount << "\n";
        }

        if (IsBettingRoundComplete()) {
            AdvanceBettingRound();
        }
        else {
            MoveTurnToNextActivePlayer();
        }
}

void GameTable::BuildDeck()
{
    m_Deck.clear();
    for (int suit = 0; suit < 4; ++suit) {
        for (int rank = 2; rank <= 14; ++rank) {
            ServerCard card;
            card.rank = rank;
            card.suit = suit;
            m_Deck.push_back(card);
        }
    }
}

void GameTable::ShuffleDeck()
{
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(m_Deck.begin(), m_Deck.end(), std::default_random_engine(seed));
}

void GameTable::DealCards()
{
    std::cout << "Dealing cards to players\n";

    for (int i = 0; i < 2; ++i) {
        for (auto& player : m_Players) {
            if (m_Deck.empty())
                return;
            ServerCard topCard = m_Deck.back();
            m_Deck.pop_back();
            
            player->DealCard(topCard);
        }
    }
}

void GameTable::CollectBlinds()
{
    if (m_Players.size() < 2)
        return;
    std::cout << "[Table] Collecting small and big blind";
    m_Players[0]->RemoveChips(SMALL_BLIND);
    if (m_Players.size() > 1) {
        m_Players[1]->RemoveChips(BIG_BLIND);
    }

    m_Pot += SMALL_BLIND + BIG_BLIND;
    m_CurrentBet = BIG_BLIND;
}

void GameTable::MoveTurnToNextActivePlayer()
{
    if (m_Players.empty())
        return;

    size_t initialScanIndex = m_CurrentTurnIdx;

    do {
        m_CurrentTurnIdx = (m_CurrentTurnIdx + 1) % m_Players.size();
        if (m_CurrentTurnIdx == initialScanIndex) break;
    } while (m_Players[m_CurrentTurnIdx]->IsFolded());

    std::cout << "Next turn is player index: " << m_CurrentTurnIdx << " (" << m_Players[m_CurrentTurnIdx]->GetUsername() << ")\n";
}

bool GameTable::IsBettingRoundComplete()
{
    m_ActionsThisRound++;
    
    int activePlayers = 0;
    for (const auto& p : m_Players) {
        if (p && !p->IsFolded()) activePlayers++;
    }

    if (m_ActionsThisRound >= activePlayers) {
        m_ActionsThisRound = 0;
        return true;
    }
    return false;
}

void GameTable::EvaluateShowdown()
{
    std::cout << "Evaluating Hands and distributing Pot: " << m_Pot << "\n";
    //Placeholder logic for now
    for (auto& player : m_Players) {
        if (player && !player->IsFolded()) {
            player->AddChips(m_Pot);
            std::cout << "[WINNER] Awarded " << m_Pot << " chips to " << player->GetUsername() << "\n";
            break;
        }
    }
}
