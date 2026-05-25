#pragma once
#include <vector>
#include <string>
#include <WinSock2.h>

enum class BettingRound {
    PRE_FLOP,
    FLOP,
    TURN,
    RIVER,
    SHOWDOWN
};

struct ServerCard {
    int rank;
    int suit;
};

enum class PlayerStatus {
    SITTING_OUT,
    ACTIVE,
    FOLDED
};
