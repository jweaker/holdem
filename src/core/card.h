#ifndef POKER_CARD_H
#define POKER_CARD_H

#include <string>

namespace poker {

enum Suit {
    SuitNone = 0,
    Clubs,
    Diamonds,
    Hearts,
    Spades
};

enum Rank {
    RankNone = 0,
    Two = 2,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Ten,
    Jack,
    Queen,
    King,
    Ace
};

struct Card {
    Suit suit;
    Rank rank;
    bool known;
};

Card makeUnknownCard();
Card makeCard(Rank rank, Suit suit);

bool sameCard(Card a, Card b);
bool isValidKnownCard(Card card);
bool parseCard(const std::string &text, Card &out);
std::string cardToString(Card card);

} // namespace poker

#endif
