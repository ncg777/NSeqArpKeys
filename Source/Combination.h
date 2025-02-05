#pragma once
#include <bitset>
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iterator>
#include <random>
#include <numeric>
#include "Sequence.h"

class Combination : public std::bitset<128> {
public:
    Combination(const Combination& c);
    Combination(int n);
    Combination();
    static int find_first(const std::bitset<128>& bitset, int n);

    static int find_next(const std::bitset<128>& bitset, int pos, int n);

    static int find_last(const std::bitset<128>& bitset, int n);

    int find_first() const;

    int find_next(int pos) const;
    int find_last() const;
    int getN() const;

    int getK() const;

    int calcSpan() const;

    Sequence getIntervalVector() const;

    Combination(int n, const std::set<int>& s);

    Combination(const std::bitset<128>& c, int n);


    Combination reverse() const;

    double calcNormalizedDistanceWith(const Combination& other) const;

    Combination symmetricDifference(const Combination& y) const;

    std::string toBinaryString() const;

    static Combination fromBinaryString(const std::string& s);

    static Combination mergeAll(const std::vector<Combination>& r);

    std::string toString() const;

    Sequence asSequence() const;

    std::set<int> asSet() const;

    Sequence asBinarySequence() const;

    static Combination fromBinarySequence(const Sequence& s);

    int compareTo(const Combination& o) const;
    bool operator<(const Combination& other) const;
    static std::vector<Combination> refinements(const Combination& c);

    static Combination merge(const Combination& a, const Combination& b);

    Combination rotate(int t) const;

    Combination intersect(const Combination& c) const;

    Combination minus(const Combination& c) const;

    std::vector<Combination> partition(const Sequence& p0) const;

    std::vector<Combination> partition(const std::vector<int>& partition) const;


    static Combination genRnd(int n);

    static Combination genRnd(int n, int k);

private:
    int m_n;
};
