#include "../src/core/card.h"
#include "../src/core/deck.h"
#include "../src/core/evaluator.h"
#include "../src/core/game_state.h"
#include "../src/core/simulation.h"

#include <cmath>
#include <iostream>
#include <list>

using namespace poker;

static int failures = 0;

static void check(bool condition, const char *message) {
    if (!condition) {
        std::cout << "FAIL: " << message << "\n";
        failures++;
    }
}

static HandValue value(Card a, Card b, Card c, Card d, Card e, Card f, Card g) {
    Card cards[7] = {a, b, c, d, e, f, g};
    return evaluateSevenCardHand(cards);
}

static void testParsingAndDeck() {
    Card card;
    check(parseCard("AS", card), "parse AS");
    check(card.known && card.rank == Ace && card.suit == Spades, "AS values");
    check(cardToString(card) == "AS", "card to string");
    check(parseCard("??", card) && !card.known, "parse unknown");
    check(!parseCard("1S", card), "reject bad rank");

    std::list<Card> deck;
    buildFullDeck(deck);
    check(deck.size() == 52, "deck has 52 cards");
    check(removeCardFromDeck(deck, makeCard(Ace, Spades)), "remove known card");
    check(deck.size() == 51, "deck has 51 after remove");
    check(!removeCardFromDeck(deck, makeCard(Ace, Spades)), "cannot remove duplicate missing card");
}

static void testHandRanking() {
    HandValue high = value(
        makeCard(Ace, Spades), makeCard(King, Hearts), makeCard(Eight, Clubs),
        makeCard(Six, Diamonds), makeCard(Four, Spades), makeCard(Three, Hearts), makeCard(Two, Clubs));
    HandValue pair = value(
        makeCard(Ace, Spades), makeCard(Ace, Hearts), makeCard(Eight, Clubs),
        makeCard(Six, Diamonds), makeCard(Four, Spades), makeCard(Three, Hearts), makeCard(Two, Clubs));
    HandValue twoPair = value(
        makeCard(Ace, Spades), makeCard(Ace, Hearts), makeCard(Eight, Clubs),
        makeCard(Eight, Diamonds), makeCard(Four, Spades), makeCard(Three, Hearts), makeCard(Two, Clubs));
    HandValue trips = value(
        makeCard(Ace, Spades), makeCard(Ace, Hearts), makeCard(Ace, Clubs),
        makeCard(Eight, Diamonds), makeCard(Four, Spades), makeCard(Three, Hearts), makeCard(Two, Clubs));
    HandValue straight = value(
        makeCard(Ace, Spades), makeCard(Five, Hearts), makeCard(Four, Clubs),
        makeCard(Three, Diamonds), makeCard(Two, Spades), makeCard(King, Hearts), makeCard(Nine, Clubs));
    HandValue flush = value(
        makeCard(Ace, Spades), makeCard(Jack, Spades), makeCard(Eight, Spades),
        makeCard(Six, Spades), makeCard(Two, Spades), makeCard(King, Hearts), makeCard(Nine, Clubs));
    HandValue fullHouse = value(
        makeCard(Ace, Spades), makeCard(Ace, Hearts), makeCard(Ace, Clubs),
        makeCard(Eight, Diamonds), makeCard(Eight, Spades), makeCard(Three, Hearts), makeCard(Two, Clubs));
    HandValue quads = value(
        makeCard(Ace, Spades), makeCard(Ace, Hearts), makeCard(Ace, Clubs),
        makeCard(Ace, Diamonds), makeCard(Eight, Spades), makeCard(Three, Hearts), makeCard(Two, Clubs));
    HandValue straightFlush = value(
        makeCard(Nine, Spades), makeCard(Eight, Spades), makeCard(Seven, Spades),
        makeCard(Six, Spades), makeCard(Five, Spades), makeCard(Ace, Hearts), makeCard(Two, Clubs));

    check(compareHandValues(pair, high) > 0, "pair beats high card");
    check(compareHandValues(twoPair, pair) > 0, "two pair beats pair");
    check(compareHandValues(trips, twoPair) > 0, "trips beats two pair");
    check(compareHandValues(straight, trips) > 0, "straight beats trips");
    check(compareHandValues(flush, straight) > 0, "flush beats straight");
    check(compareHandValues(fullHouse, flush) > 0, "full house beats flush");
    check(compareHandValues(quads, fullHouse) > 0, "quads beats full house");
    check(compareHandValues(straightFlush, quads) > 0, "straight flush beats quads");
    check(straight.category == Straight && straight.ranks[0] == Five, "wheel straight high is five");
}

static void testValidationAndSimulation() {
    GameState duplicate = makeEmptyGameState(2);
    duplicate.players[0].first = makeCard(Ace, Spades);
    duplicate.players[1].first = makeCard(Ace, Spades);
    char error[128];
    check(!validateGameState(duplicate, error), "duplicate card rejected");

    GameState state = makeEmptyGameState(2);
    state.players[0].first = makeCard(Ace, Spades);
    state.players[0].second = makeCard(Ace, Hearts);
    state.players[1].first = makeCard(King, Spades);
    state.players[1].second = makeCard(King, Hearts);

    OddsResult a = calculateOdds(state, 1000, 1234);
    OddsResult b = calculateOdds(state, 1000, 1234);
    check(a.valid, "simulation result valid");
    check(a.simulationsRun == 1000, "simulation count");
    check(std::fabs(a.winPct[0] - b.winPct[0]) < 0.00001, "seeded win result stable");
    check(std::fabs(a.tiePct[0] - b.tiePct[0]) < 0.00001, "seeded tie result stable");
    check(a.winPct[0] > a.winPct[1], "aces favored over kings preflop");

    GameState tie = makeEmptyGameState(2);
    tie.players[0].first = makeCard(Two, Clubs);
    tie.players[0].second = makeCard(Three, Diamonds);
    tie.players[1].first = makeCard(Four, Clubs);
    tie.players[1].second = makeCard(Five, Diamonds);
    tie.boardCount = 5;
    tie.board[0] = makeCard(Ace, Spades);
    tie.board[1] = makeCard(King, Spades);
    tie.board[2] = makeCard(Queen, Spades);
    tie.board[3] = makeCard(Jack, Spades);
    tie.board[4] = makeCard(Ten, Spades);
    OddsResult tieResult = calculateOdds(tie, 10, 99);
    check(tieResult.valid, "tie result valid");
    check(tieResult.winPct[0] == 0.0 && tieResult.winPct[1] == 0.0, "board royal has no solo wins");
    check(tieResult.tiePct[0] == 100.0 && tieResult.tiePct[1] == 100.0, "board royal ties both players");
}

int main() {
    testParsingAndDeck();
    testHandRanking();
    testValidationAndSimulation();

    if (failures == 0) {
        std::cout << "All core tests passed.\n";
        return 0;
    }

    std::cout << failures << " test(s) failed.\n";
    return 1;
}
