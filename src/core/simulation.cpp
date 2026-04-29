#include "simulation.h"

#include "deck.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <deque>
#include <list>
#include <random>

namespace poker {

static OddsResult makeEmptyResult() {
    OddsResult result;
    for (int i = 0; i < MaxPlayers; ++i) {
        result.winPct[i] = 0.0;
        result.tiePct[i] = 0.0;
    }
    result.simulationsRun = 0;
    result.valid = true;
    result.error[0] = '\0';
    return result;
}

static void setError(char error[128], const char *message) {
    std::snprintf(error, 128, "%s", message);
}

bool validateGameState(const GameState &state, char error[128]) {
    error[0] = '\0';
    if (state.playerCount < 2 || state.playerCount > MaxPlayers) {
        setError(error, "playerCount must be between 2 and 6");
        return false;
    }

    if (state.boardCount < 0 || state.boardCount > BoardCards) {
        setError(error, "boardCount must be between 0 and 5");
        return false;
    }

    int activeCount = 0;
    Card seen[MaxPlayers * HoleCards + BoardCards];
    int seenCount = 0;

    for (int i = 0; i < state.playerCount; ++i) {
        if (!state.players[i].active) {
            continue;
        }

        activeCount++;
        Card hole[2] = {state.players[i].first, state.players[i].second};
        for (int h = 0; h < 2; ++h) {
            if (!hole[h].known) {
                continue;
            }
            if (!isValidKnownCard(hole[h])) {
                setError(error, "invalid known player card");
                return false;
            }
            for (int s = 0; s < seenCount; ++s) {
                if (sameCard(seen[s], hole[h])) {
                    setError(error, "duplicate card found");
                    return false;
                }
            }
            seen[seenCount++] = hole[h];
        }
    }

    if (activeCount < 2) {
        setError(error, "at least two active players are required");
        return false;
    }

    for (int i = 0; i < state.boardCount; ++i) {
        if (!state.board[i].known || !isValidKnownCard(state.board[i])) {
            setError(error, "board cards before boardCount must be known and valid");
            return false;
        }
        for (int s = 0; s < seenCount; ++s) {
            if (sameCard(seen[s], state.board[i])) {
                setError(error, "duplicate card found");
                return false;
            }
        }
        seen[seenCount++] = state.board[i];
    }

    return true;
}

static void removeKnownCards(const GameState &state, std::list<Card> &deck) {
    for (int i = 0; i < state.playerCount; ++i) {
        if (!state.players[i].active) {
            continue;
        }
        if (state.players[i].first.known) removeCardFromDeck(deck, state.players[i].first);
        if (state.players[i].second.known) removeCardFromDeck(deck, state.players[i].second);
    }

    for (int i = 0; i < state.boardCount; ++i) {
        if (state.board[i].known) {
            removeCardFromDeck(deck, state.board[i]);
        }
    }
}

static void fillTrialCards(GameState &trial, std::deque<Card> &drawPile) {
    for (int i = 0; i < trial.playerCount; ++i) {
        if (!trial.players[i].active) {
            continue;
        }
        if (!trial.players[i].first.known) trial.players[i].first = drawCard(drawPile);
        if (!trial.players[i].second.known) trial.players[i].second = drawCard(drawPile);
    }

    for (int i = trial.boardCount; i < BoardCards; ++i) {
        trial.board[i] = drawCard(drawPile);
    }
}

static HandValue evaluatePlayer(const GameState &trial, int playerIndex) {
    Card cards[7];
    cards[0] = trial.players[playerIndex].first;
    cards[1] = trial.players[playerIndex].second;
    for (int i = 0; i < BoardCards; ++i) {
        cards[i + 2] = trial.board[i];
    }
    return evaluateSevenCardHand(cards);
}

OddsResult calculateOdds(GameState state, int simulations) {
    unsigned int seed = static_cast<unsigned int>(std::time(0));
    return calculateOdds(state, simulations, seed);
}

OddsResult calculateOdds(GameState state, int simulations, unsigned int seed) {
    OddsResult result = makeEmptyResult();
    if (simulations <= 0) {
        simulations = 1;
    }

    if (!validateGameState(state, result.error)) {
        result.valid = false;
        return result;
    }

    std::list<Card> baseDeck;
    buildFullDeck(baseDeck);
    removeKnownCards(state, baseDeck);

    double wins[MaxPlayers] = {0.0};
    double ties[MaxPlayers] = {0.0};
    std::mt19937 rng(seed);

    for (int run = 0; run < simulations; ++run) {
        std::deque<Card> drawPile;
        shuffleDeckToDrawPile(baseDeck, drawPile, rng);

        GameState trial = state;
        fillTrialCards(trial, drawPile);

        HandValue best;
        best.category = HighCard;
        for (int i = 0; i < 5; ++i) {
            best.ranks[i] = 0;
        }
        bool hasBest = false;
        int winners[MaxPlayers] = {0};
        int winnerCount = 0;

        for (int i = 0; i < trial.playerCount; ++i) {
            if (!trial.players[i].active) {
                continue;
            }

            HandValue current = evaluatePlayer(trial, i);
            int cmp = hasBest ? compareHandValues(current, best) : 1;
            if (cmp > 0) {
                best = current;
                hasBest = true;
                winnerCount = 0;
                winners[winnerCount++] = i;
            } else if (cmp == 0) {
                winners[winnerCount++] = i;
            }
        }

        if (winnerCount == 1) {
            wins[winners[0]] += 1.0;
        } else {
            for (int i = 0; i < winnerCount; ++i) {
                ties[winners[i]] += 1.0;
            }
        }

        result.simulationsRun++;
    }

    for (int i = 0; i < MaxPlayers; ++i) {
        result.winPct[i] = 100.0 * wins[i] / result.simulationsRun;
        result.tiePct[i] = 100.0 * ties[i] / result.simulationsRun;
    }

    return result;
}

} // namespace poker
