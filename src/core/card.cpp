#include "card.h"

#include <cctype>

namespace poker {

Card makeUnknownCard() {
    Card card;
    card.suit = SuitNone;
    card.rank = RankNone;
    card.known = false;
    return card;
}

Card makeCard(Rank rank, Suit suit) {
    Card card;
    card.suit = suit;
    card.rank = rank;
    card.known = true;
    return card;
}

bool sameCard(Card a, Card b) {
    return a.known && b.known && a.rank == b.rank && a.suit == b.suit;
}

bool isValidKnownCard(Card card) {
    return card.known && card.rank >= Two && card.rank <= Ace
        && card.suit >= Clubs && card.suit <= Spades;
}

static bool parseRank(char value, Rank &rank) {
    value = static_cast<char>(std::toupper(static_cast<unsigned char>(value)));
    if (value >= '2' && value <= '9') {
        rank = static_cast<Rank>(value - '0');
        return true;
    }

    if (value == 'T') rank = Ten;
    else if (value == 'J') rank = Jack;
    else if (value == 'Q') rank = Queen;
    else if (value == 'K') rank = King;
    else if (value == 'A') rank = Ace;
    else return false;

    return true;
}

static bool parseSuit(char value, Suit &suit) {
    value = static_cast<char>(std::toupper(static_cast<unsigned char>(value)));
    if (value == 'C') suit = Clubs;
    else if (value == 'D') suit = Diamonds;
    else if (value == 'H') suit = Hearts;
    else if (value == 'S') suit = Spades;
    else return false;

    return true;
}

bool parseCard(const std::string &text, Card &out) {
    if (text == "??" || text == "--" || text == "") {
        out = makeUnknownCard();
        return true;
    }

    if (text.size() != 2) {
        return false;
    }

    Rank rank = RankNone;
    Suit suit = SuitNone;
    if (!parseRank(text[0], rank) || !parseSuit(text[1], suit)) {
        return false;
    }

    out = makeCard(rank, suit);
    return true;
}

std::string cardToString(Card card) {
    if (!card.known) {
        return "??";
    }

    std::string text;
    if (card.rank >= Two && card.rank <= Nine) text += static_cast<char>('0' + card.rank);
    else if (card.rank == Ten) text += 'T';
    else if (card.rank == Jack) text += 'J';
    else if (card.rank == Queen) text += 'Q';
    else if (card.rank == King) text += 'K';
    else if (card.rank == Ace) text += 'A';
    else text += '?';

    if (card.suit == Clubs) text += 'C';
    else if (card.suit == Diamonds) text += 'D';
    else if (card.suit == Hearts) text += 'H';
    else if (card.suit == Spades) text += 'S';
    else text += '?';

    return text;
}

} // namespace poker
