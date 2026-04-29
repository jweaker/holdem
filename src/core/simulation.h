#ifndef POKER_SIMULATION_H
#define POKER_SIMULATION_H

#include "game_state.h"
#include "evaluator.h"

namespace poker {

bool validateGameState(const GameState &state, char error[128]);
OddsResult calculateOdds(GameState state, int simulations);
OddsResult calculateOdds(GameState state, int simulations, unsigned int seed);

} // namespace poker

#endif
