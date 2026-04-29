#include "core/card.h"
#include "core/game_state.h"
#include "core/simulation.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace poker;

static void printUsage() {
    std::cout
        << "Texas Hold'em odds calculator\n"
        << "Usage:\n"
        << "  ./poker_engine [simulations]\n\n"
        << "Then enter cards like AS, TD, 7H, or ?? for unknown.\n";
}

static Card askCard(const std::string &label) {
    while (true) {
        std::cout << label << ": ";
        std::string text;
        std::cin >> text;

        Card card;
        if (parseCard(text, card)) {
            return card;
        }

        std::cout << "Invalid card. Use rank 2-9,T,J,Q,K,A and suit C,D,H,S. Example: AS\n";
    }
}

int main(int argc, char **argv) {
    int simulations = 10000;
    if (argc > 1) {
        simulations = std::atoi(argv[1]);
    }

    printUsage();

    int playerCount = 2;
    std::cout << "Players (2-6): ";
    std::cin >> playerCount;
    if (playerCount < 2) playerCount = 2;
    if (playerCount > MaxPlayers) playerCount = MaxPlayers;

    GameState state = makeEmptyGameState(playerCount);

    for (int i = 0; i < playerCount; ++i) {
        std::cout << "\nPlayer " << (i + 1) << "\n";
        state.players[i].first = askCard("  Card 1");
        state.players[i].second = askCard("  Card 2");
    }

    std::cout << "\nKnown board cards (0-5): ";
    std::cin >> state.boardCount;
    if (state.boardCount < 0) state.boardCount = 0;
    if (state.boardCount > BoardCards) state.boardCount = BoardCards;

    for (int i = 0; i < state.boardCount; ++i) {
        state.board[i] = askCard("  Board card " + std::to_string(i + 1));
    }

    OddsResult result = calculateOdds(state, simulations);
    if (!result.valid) {
        std::cout << "\nError: " << result.error << "\n";
        return 1;
    }

    std::cout << "\nSimulations: " << result.simulationsRun << "\n";
    for (int i = 0; i < playerCount; ++i) {
        std::cout << "Player " << (i + 1)
                  << "  win: " << result.winPct[i] << "%"
                  << "  tie: " << result.tiePct[i] << "%\n";
    }

    return 0;
}
