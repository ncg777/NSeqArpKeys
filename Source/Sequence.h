#pragma once

#include <vector>
#include <set>
#include <map>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iterator>
#include <numeric>
#include <random>
#include <functional>
#include <iostream>
#include "HomoPair.h"
#include "Numbers.h"
#include "RandomNumberGenerator.h"

class Sequence : public std::vector<int>, public std::function<int(int)> {
public:
    using std::vector<int>::vector;

    bool isNatural() const;
    Sequence(const std::vector<int>& p_arr);
    std::vector<double> getSymmetries() const;

    enum class ArpType {
        UP,
        DOWN,
        UPDOWN,
        DOWNUP,
        SIGMAUP,
        SIGMADOWN
    };

    static Sequence arp(ArpType arpType, int k, bool repeatBottom = false, bool repeatTop = false);

    static Sequence parse(const std::string& s);

    static Sequence stair(int o, int l, int a);

    static Sequence tri(int o, int l, int a);

    Sequence getMininumRotation() const;

    Sequence convolveWith(const Sequence& impulse) const;

    Sequence juxtapose(const Sequence& j) const;

    Sequence wrapseq(int p_min, int p_amp) const;

    Sequence bounceseq(int p_min, int p_amp) const;

    Sequence flip() const;

    static Sequence subFlip(const Sequence& s, int l, int h);

    std::map<int, int> frequencyMap() const;

    double entropy() const;

    int sumOfPairwiseDistances() const;

    Sequence signs() const;

    Sequence multiply(int k) const;

    Sequence applyMin(int k) const;

    Sequence applyMax(int k) const;

    Sequence apply(const std::function<int(int)>& f) const;

    Sequence powExp(int power) const;

    Sequence addToEach(const Sequence& s) const;

    Sequence addToEach(int k) const;

    Sequence powBase(int base) const;

    Sequence reverse() const;

    Sequence rotateRight() const;

    Sequence rotateLeft() const;

    Sequence rotate(int n) const;

    std::vector<int> getArray() const;

    int getMin() const;

    int getMax() const;

    double getMean() const;

    double getStdDev() const;

    Sequence difference() const;

    Sequence cyclicalDifference() const;

    Sequence antidifference(int k) const;

    Sequence cyclicalAntidifference(int k) const;

    static Sequence from(const std::vector<int>& p_arr);

    std::set<int> distinct() const;

    int count(int n) const;

    bool operator==(const Sequence& other) const;

    bool operator!=(const Sequence& other) const;

    bool operator<(const Sequence& other) const;

    int compareTo(const Sequence& o) const;

    std::string toString(bool asJson = false) const;

    int sum() const;

    int getPeriod() const;

    Sequence circularHoldNonZero() const;

    int rangeSize() const;

    std::map<int, int> mapOrdinalsUnipolar() const;

    Sequence asOrdinalsUnipolar() const;

    std::map<int, int> mapOrdinalsBipolar() const;

    Sequence asOrdinalsBipolar() const;

    Sequence map(const Sequence& s) const;

    Sequence map(const std::map<int, int>& i) const;

    Sequence permutate(const Sequence& s) const;

    void addAtRandom(int v);

    Sequence copy() const;

    Sequence rndRemove(int n) const;

    Sequence rndAdd(int n) const;

    static Sequence map(const Sequence& s, const std::map<int, int>& m);
    
    static int equivalenceShift(const Sequence& a, const Sequence& b);

    static bool equivalentUnderRotation(const Sequence& a, const Sequence& b);

    class ReverseComparator {
    public:
        int operator()(const Sequence& o1, const Sequence& o2) const;
    };

    int operator()(int t) const;
};
