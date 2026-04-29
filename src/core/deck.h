#ifndef POKER_DECK_H
#define POKER_DECK_H

#include "card.h"

#include <deque>
#include <list>
#include <random>

namespace poker {

void buildFullDeck(std::list<Card> &deck);
bool removeCardFromDeck(std::list<Card> &deck, Card card);
void shuffleDeckToDrawPile(const std::list<Card> &deck, std::deque<Card> &drawPile, std::mt19937 &rng);
Card drawCard(std::deque<Card> &drawPile);

} // namespace poker

#endif
