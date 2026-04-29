#ifndef POKER_EVALUATOR_H
#define POKER_EVALUATOR_H

#include "card.h"

namespace poker {

enum HandCategory {
    HighCard = 0,
    OnePair,
    TwoPair,
    ThreeOfKind,
    Straight,
    Flush,
    FullHouse,
    FourOfKind,
    StraightFlush
};

struct HandValue {
    HandCategory category;
    int ranks[5];
};

HandValue evaluateSevenCardHand(Card cards[7]);
int compareHandValues(HandValue a, HandValue b);
const char *handCategoryName(HandCategory category);

} // namespace poker

#endif
