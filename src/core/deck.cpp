#include "deck.h"

#include <algorithm>

namespace poker {

void buildFullDeck(std::list<Card> &deck) {
    deck.clear();
    for (int suit = Clubs; suit <= Spades; ++suit) {
        for (int rank = Two; rank <= Ace; ++rank) {
            deck.push_back(makeCard(static_cast<Rank>(rank), static_cast<Suit>(suit)));
        }
    }
}

bool removeCardFromDeck(std::list<Card> &deck, Card card) {
    for (std::list<Card>::iterator it = deck.begin(); it != deck.end(); ++it) {
        if (sameCard(*it, card)) {
            deck.erase(it);
            return true;
        }
    }

    return false;
}

void shuffleDeckToDrawPile(const std::list<Card> &deck, std::deque<Card> &drawPile, std::mt19937 &rng) {
    drawPile.clear();
    for (std::list<Card>::const_iterator it = deck.begin(); it != deck.end(); ++it) {
        drawPile.push_back(*it);
    }

    std::shuffle(drawPile.begin(), drawPile.end(), rng);
}

Card drawCard(std::deque<Card> &drawPile) {
    Card card = drawPile.back();
    drawPile.pop_back();
    return card;
}

} // namespace poker
