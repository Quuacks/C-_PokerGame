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
    m_ActionsThisRound = 0;
    m_CurrentBet = 0;
    for (auto& player : m_Players)
    {
        if (player)
            player->m_CurrentRoundBet = 0;
    }

    if (m_Round == BettingRound::PRE_FLOP) {
        m_Round = BettingRound::FLOP;
        m_TableState = TableState::FLOP;
        std::cout << "Moving To Flop";
        
        for (int i = 0; i < 3; ++i) {
            m_CommunityCards.push_back(m_Deck.back());
            m_Deck.pop_back();
        }
    }
    else if (m_Round == BettingRound::FLOP) {
        m_Round = BettingRound::TURN;
        m_TableState = TableState::TURN;
        std::cout << "Moving To Turn\n";
        m_CommunityCards.push_back(m_Deck.back());
        m_Deck.pop_back();
    }
    else if (m_Round == BettingRound::TURN) {
        m_Round = BettingRound::RIVER;
        m_TableState = TableState::RIVER;
        std::cout << "Moving to river";
        m_CommunityCards.push_back(m_Deck.back());
        m_Deck.pop_back();
    }
    else if (m_Round == BettingRound::RIVER) {
        m_Round = BettingRound::SHOWDOWN;
        m_TableState = TableState::SHOWDOWN;
        std::cout << "Moving to Showdown";
    }

}

void GameTable::ProcessPlayerAction(SOCKET socket, const std::string& actionType, int amount)
{
    if (m_Players.empty())
        return;

    auto activePlayer = m_Players[m_CurrentTurnIdx];
    if (!activePlayer || activePlayer->getSocket() != socket) {
        std::cout << "[Warning] Player tried to act out of turn";
        return;
    }

    auto countActivePlayers = [&]() -> int {
        int count = 0;
        for (const auto& p : m_Players) {
            if (p && !p->IsFolded())
                count++;
        }
        return count;
    };

    auto findFirstActiveIndex = [&]() -> size_t {
        for (size_t i = 0; i < m_Players.size(); ++i) {
            if (m_Players[i] && !m_Players[i]->IsFolded())
                return i;
        }
        return 0;
    };

    bool handled = false;
    if (actionType == "FOLD") {
        std::cout << "[ACTION] " << activePlayer->GetUsername() << " folds.\n";
        activePlayer->SetFolded(true);
        handled = true;
    }
    else if (actionType == "CALL") {
        int callAmount = m_CurrentBet - activePlayer->m_CurrentRoundBet;
        if (callAmount < 0)
            callAmount = 0;

        if (callAmount > 0) {
            m_Pot += callAmount;
            activePlayer->RemoveChips(callAmount);
            activePlayer->m_CurrentRoundBet += callAmount;
        }
        std::cout << "[ACTION] " << activePlayer->GetUsername() << " calls " << callAmount << "\n";
        handled = true;
    }
    else if (actionType == "CHECK") {
        // If there's something to call, treat CHECK like CALL.
        if (m_CurrentBet > activePlayer->m_CurrentRoundBet) {
            int callAmount = m_CurrentBet - activePlayer->m_CurrentRoundBet;
            if (callAmount > 0) {
                m_Pot += callAmount;
                activePlayer->RemoveChips(callAmount);
                activePlayer->m_CurrentRoundBet += callAmount;
            }
            std::cout << "[ACTION] " << activePlayer->GetUsername() << " checks/calls " << callAmount << "\n";
        }
        else {
            std::cout << "[ACTION] " << activePlayer->GetUsername() << " checks.\n";
        }
        handled = true;
    }
    else if (actionType == "RAISE") {
        // Client sends "raise amount" as an ADDITIONAL amount to put in on top of the player's current bet.
        int raiseAdd = amount;
        if (raiseAdd < 0)
            raiseAdd = 0;

        if (raiseAdd == 0) {
            // If player raises 0, fall back to CALL semantics.
            int callAmount = m_CurrentBet - activePlayer->m_CurrentRoundBet;
            if (callAmount < 0)
                callAmount = 0;

            if (callAmount > 0) {
                m_Pot += callAmount;
                activePlayer->RemoveChips(callAmount);
                activePlayer->m_CurrentRoundBet += callAmount;
            }
            std::cout << "[ACTION] " << activePlayer->GetUsername() << " calls " << callAmount << "\n";
        }
        else {
            m_Pot += raiseAdd;
            activePlayer->RemoveChips(raiseAdd);
            activePlayer->m_CurrentRoundBet += raiseAdd;
            m_CurrentBet = activePlayer->m_CurrentRoundBet;
            std::cout << "[ACTION] " << activePlayer->GetUsername() << " raises (add) " << raiseAdd
                << " to " << m_CurrentBet << "\n";
        }
        handled = true;
    }

    if (!handled) {
        std::cout << "[Warning] Unknown action type: " << actionType << "\n";
        return;
    }

    m_ActionsThisRound++;

    // If only one active player remains, finish the hand immediately.
    if (countActivePlayers() <= 1) {
        m_Round = BettingRound::SHOWDOWN;
        m_TableState = TableState::SHOWDOWN;
        m_CurrentTurnIdx = findFirstActiveIndex();
        return;
    }

    if (IsBettingRoundComplete()) {
        AdvanceBettingRound();
        MoveTurnToNextActivePlayer();
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
    m_ActionsThisRound = 0;
    std::cout << "[Table] Collecting small and big blind\n";
    for (auto& player : m_Players) {
        if (player)
            player->m_CurrentRoundBet = 0;
    }

    // Posting blinds (index 0 = small blind, index 1 = big blind).
    m_Players[0]->RemoveChips(SMALL_BLIND);
    m_Players[0]->m_CurrentRoundBet = SMALL_BLIND;

    if (m_Players.size() > 1) {
        m_Players[1]->RemoveChips(BIG_BLIND);
        m_Players[1]->m_CurrentRoundBet = BIG_BLIND;
    }

    m_Pot += SMALL_BLIND + BIG_BLIND;
    m_CurrentBet = BIG_BLIND;

    // Pre-flop: heads-up => small blind acts first; 3+ players => next after big blind.
    if (m_Players.size() == 2) {
        m_CurrentTurnIdx = 0;
    }
    else {
        m_CurrentTurnIdx = 2 % m_Players.size();
    }
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
    int activePlayers = 0;
    int playersMatchedBet = 0;

    for (const auto& p : m_Players)
    {
        if (!p || p->IsFolded())
            continue;

        activePlayers++;

        if (p->m_CurrentRoundBet == m_CurrentBet)
            playersMatchedBet++;
    }

    // Only one player left
    if (activePlayers <= 1)
        return true;

    // Everyone has matched the bet
    if (playersMatchedBet == activePlayers && m_ActionsThisRound >= static_cast<size_t>(activePlayers))
    {
        
        return m_ActionsThisRound >= activePlayers;
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

void GameTable::Tick()
{
    switch (m_TableState)
    {
    case TableState::WAITING_FOR_PLAYERS:
    {
        if (m_Players.size() >= 2)
        {
            std::cout << "[Table] Enough players. Starting hand.\n";
            m_TableState = TableState::STARTING_HAND;
        }
        break;
    }

    case TableState::STARTING_HAND:
    {
        StartNewHand();
        m_TableState = TableState::PRE_FLOP;
        break;
    }

    case TableState::PRE_FLOP:
    {
        break;
    }
    case TableState::FLOP:
    {
        break;
    }
    case TableState::TURN:
    {
        break;
    }
    case TableState::RIVER:
    {
        // Waiting for player actions
        break;
    }

    case TableState::SHOWDOWN:
    {
        EvaluateShowdown();

        m_TableState = TableState::HAND_FINISHED;
        break;
    }

    case TableState::HAND_FINISHED:
    {
        StartNewHand();
        m_TableState = TableState::PRE_FLOP;
        break;
    }
    }
}
