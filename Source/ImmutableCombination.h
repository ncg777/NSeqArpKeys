#pragma once

#include <bitset>
#include <set>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include "Combination.h"
#include "Sequence.h"

class ImmutableCombination {
public:
    ImmutableCombination(const ImmutableCombination& other) : combination(other.combination) {}

    ImmutableCombination(const Combination& c) : combination(c) {}
    ImmutableCombination(int p_n) : combination(p_n) {}

    ImmutableCombination(int p_n, const std::set<int>& p_s) : combination(p_n, p_s) {}

    ImmutableCombination(const std::bitset<128>& c, int n) : combination(c, n) {}

    static ImmutableCombination fromCombination(const Combination& c) {
        return ImmutableCombination(Combination(c), c.getN());
    }

    int getN() const {
        return combination.getN();
    }

    int getK() const {
        return combination.getK();
    }

    int calcSpan() const {
        return combination.calcSpan();
    }

    Sequence getIntervalVector() const {
        return combination.getIntervalVector();
    }

    double calcNormalizedDistanceWith(const ImmutableCombination& other) const {
        return combination.calcNormalizedDistanceWith(other.combination);
    }

    ImmutableCombination symmetricDifference(const ImmutableCombination& y) const {
        return ImmutableCombination(combination.symmetricDifference(y.combination), combination.getN());
    }

    ImmutableCombination rotate(int t) const {
        return ImmutableCombination(combination.rotate(t), combination.getN());
    }

    ImmutableCombination intersect(const ImmutableCombination& c) const {
        return ImmutableCombination(combination.intersect(c.combination), combination.getN());
    }

    ImmutableCombination minus(const ImmutableCombination& c) const {
        return ImmutableCombination(combination.minus(c.combination), combination.getN());
    }

    std::vector<ImmutableCombination> partition(const std::vector<int>& partition) const {
        std::vector<Combination> result = combination.partition(partition);
        return toImmutableCombinations(result);
    }

    std::vector<ImmutableCombination> partition(const Sequence& p0) const {
        std::vector<Combination> result = combination.partition(p0);
        return toImmutableCombinations(result);
    }

    static std::vector<ImmutableCombination> toImmutableCombinations(const std::vector<Combination>& combinations) {
        std::vector<ImmutableCombination> immutableCombinations(combinations.size());
        for (size_t i = 0; i < combinations.size(); ++i) {
            immutableCombinations[i] = ImmutableCombination(combinations[i], combinations[i].getN());
        }
        return immutableCombinations;
    }

    Sequence asSequence() const {
        return combination.asSequence();
    }

    std::set<int> asSet() const {
        return combination.asSet();
    }

    Sequence asBinarySequence() const {
        return combination.asBinarySequence();
    }

    Composition getComposition() const {
        return combination.getComposition();
    }

    ImmutableCombination merge(const ImmutableCombination& other) const {
        return ImmutableCombination(Combination::merge(combination, other.combination), combination.getN());
    }

    static ImmutableCombination fromBinarySequence(const Sequence& s) {
        Combination c = Combination::fromBinarySequence(s);
        return ImmutableCombination(c, c.getN());
    }

    bool get(int bitIndex) const {
        return combination.test(bitIndex);
    }

    bool isEmpty() const {
        return combination.none();
    }

    std::string toString() const {
        return combination.toString();
    }

    Combination getCombinationCopy() const {
        return Combination(combination);
    }

    bool operator==(const ImmutableCombination& other) const {
        return combination == other.combination;
    }

    bool operator!=(const ImmutableCombination& other) const {
        return !(*this == other);
    }

    int compareTo(const ImmutableCombination& o) const {
        return combination.compareTo(o.combination);
    }

protected:
    Combination combination;
};

