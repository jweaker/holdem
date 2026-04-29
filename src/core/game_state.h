#ifndef POKER_GAME_STATE_H
#define POKER_GAME_STATE_H

#include "card.h"

namespace poker {

const int MaxPlayers = 6;
const int BoardCards = 5;
const int HoleCards = 2;

struct PlayerHand {
    Card first;
    Card second;
    bool active;
};

struct GameState {
    PlayerHand players[MaxPlayers];
    Card board[BoardCards];
    int playerCount;
    int boardCount;
};

struct OddsResult {
    double winPct[MaxPlayers];
    double tiePct[MaxPlayers];
    int simulationsRun;
    bool valid;
    char error[128];
};

GameState makeEmptyGameState(int playerCount);

} // namespace poker

#endif
