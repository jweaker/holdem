#include "game_state.h"

namespace poker {

GameState makeEmptyGameState(int playerCount) {
    GameState state;
    state.playerCount = playerCount;
    state.boardCount = 0;

    for (int i = 0; i < MaxPlayers; ++i) {
        state.players[i].first = makeUnknownCard();
        state.players[i].second = makeUnknownCard();
        state.players[i].active = i < playerCount;
    }

    for (int i = 0; i < BoardCards; ++i) {
        state.board[i] = makeUnknownCard();
    }

    return state;
}

} // namespace poker
