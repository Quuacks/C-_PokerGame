#pragma once
#include "GameStructures.h"
#include <vector>
#include <memory>
#include <algorithm>

class Player;

struct Card
{
    int rank; //2-14
    int suit; //0-3
};

class GameTable
{
    private:
        std::vector<std::shared_ptr<Player>> m_Players;
        std::vector<ServerCard> m_Deck;
        std::vector<ServerCard> m_CommunityCards;

        int m_Pot = 0;
        int m_CurrentBet = 0;
        size_t m_CurrentTurnIdx = 0;
        BettingRound m_Round = BettingRound::PRE_FLOP;

        void BuildDeck();
        void ShuffleDeck();
        void DealCards();
        void CollectBlinds();
        void MoveTurnToNextActivePlayer();
        bool IsBettingRoundComplete();
        void EvaluateShowdown();

    public:
        void AddPlayer(std::shared_ptr<Player> player);
        void RemovePlayer(SOCKET socket);

        void StartNewHand();
        void AdvanceBettingRound();
        void ProcessPlayerAction(SOCKET socket, const std::string& actionType, int amount = 0);

        int SMALL_BLIND = 10;
        int BIG_BLIND = 20;

};

