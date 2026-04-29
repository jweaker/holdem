#include "evaluator.h"

namespace poker {

static HandValue makeEmptyValue() {
    HandValue value;
    value.category = HighCard;
    for (int i = 0; i < 5; ++i) {
        value.ranks[i] = 0;
    }
    return value;
}

int compareHandValues(HandValue a, HandValue b) {
    if (a.category < b.category) return -1;
    if (a.category > b.category) return 1;

    for (int i = 0; i < 5; ++i) {
        if (a.ranks[i] < b.ranks[i]) return -1;
        if (a.ranks[i] > b.ranks[i]) return 1;
    }

    return 0;
}

static void sortDescending(int values[], int count) {
    for (int i = 0; i < count - 1; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (values[j] > values[i]) {
                int tmp = values[i];
                values[i] = values[j];
                values[j] = tmp;
            }
        }
    }
}

static int straightHighFromPresent(bool present[]) {
    if (present[Ace] && present[Five] && present[Four] && present[Three] && present[Two]) {
        return Five;
    }

    for (int high = Ace; high >= Six; --high) {
        if (present[high] && present[high - 1] && present[high - 2] && present[high - 3] && present[high - 4]) {
            return high;
        }
    }

    return 0;
}

static HandValue evaluateFiveCardHand(Card cards[5]) {
    HandValue value = makeEmptyValue();
    int counts[15] = {0};
    bool present[15] = {false};
    int ranks[5];
    bool flush = true;

    for (int i = 0; i < 5; ++i) {
        int rank = cards[i].rank;
        ranks[i] = rank;
        counts[rank]++;
        present[rank] = true;
        if (cards[i].suit != cards[0].suit) {
            flush = false;
        }
    }

    sortDescending(ranks, 5);
    int straightHigh = straightHighFromPresent(present);

    if (flush && straightHigh > 0) {
        value.category = StraightFlush;
        value.ranks[0] = straightHigh;
        return value;
    }

    int fourRank = 0;
    int threeRank = 0;
    int pairRanks[2] = {0, 0};
    int pairCount = 0;

    for (int rank = Ace; rank >= Two; --rank) {
        if (counts[rank] == 4) fourRank = rank;
        else if (counts[rank] == 3) threeRank = rank;
        else if (counts[rank] == 2 && pairCount < 2) pairRanks[pairCount++] = rank;
    }

    if (fourRank > 0) {
        value.category = FourOfKind;
        value.ranks[0] = fourRank;
        for (int i = 0; i < 5; ++i) {
            if (ranks[i] != fourRank) {
                value.ranks[1] = ranks[i];
                break;
            }
        }
        return value;
    }

    if (threeRank > 0 && pairCount > 0) {
        value.category = FullHouse;
        value.ranks[0] = threeRank;
        value.ranks[1] = pairRanks[0];
        return value;
    }

    if (flush) {
        value.category = Flush;
        for (int i = 0; i < 5; ++i) {
            value.ranks[i] = ranks[i];
        }
        return value;
    }

    if (straightHigh > 0) {
        value.category = Straight;
        value.ranks[0] = straightHigh;
        return value;
    }

    if (threeRank > 0) {
        value.category = ThreeOfKind;
        value.ranks[0] = threeRank;
        int out = 1;
        for (int i = 0; i < 5; ++i) {
            if (ranks[i] != threeRank) {
                value.ranks[out++] = ranks[i];
            }
        }
        return value;
    }

    if (pairCount == 2) {
        value.category = TwoPair;
        value.ranks[0] = pairRanks[0];
        value.ranks[1] = pairRanks[1];
        for (int i = 0; i < 5; ++i) {
            if (ranks[i] != pairRanks[0] && ranks[i] != pairRanks[1]) {
                value.ranks[2] = ranks[i];
                break;
            }
        }
        return value;
    }

    if (pairCount == 1) {
        value.category = OnePair;
        value.ranks[0] = pairRanks[0];
        int out = 1;
        for (int i = 0; i < 5; ++i) {
            if (ranks[i] != pairRanks[0]) {
                value.ranks[out++] = ranks[i];
            }
        }
        return value;
    }

    value.category = HighCard;
    for (int i = 0; i < 5; ++i) {
        value.ranks[i] = ranks[i];
    }
    return value;
}

HandValue evaluateSevenCardHand(Card cards[7]) {
    HandValue best = makeEmptyValue();
    bool first = true;

    for (int a = 0; a < 3; ++a) {
        for (int b = a + 1; b < 4; ++b) {
            for (int c = b + 1; c < 5; ++c) {
                for (int d = c + 1; d < 6; ++d) {
                    for (int e = d + 1; e < 7; ++e) {
                        Card five[5] = {cards[a], cards[b], cards[c], cards[d], cards[e]};
                        HandValue current = evaluateFiveCardHand(five);
                        if (first || compareHandValues(current, best) > 0) {
                            best = current;
                            first = false;
                        }
                    }
                }
            }
        }
    }

    return best;
}

const char *handCategoryName(HandCategory category) {
    switch (category) {
        case StraightFlush: return "Straight flush";
        case FourOfKind: return "Four of a kind";
        case FullHouse: return "Full house";
        case Flush: return "Flush";
        case Straight: return "Straight";
        case ThreeOfKind: return "Three of a kind";
        case TwoPair: return "Two pair";
        case OnePair: return "One pair";
        case HighCard: return "High card";
    }

    return "Unknown";
}

} // namespace poker
