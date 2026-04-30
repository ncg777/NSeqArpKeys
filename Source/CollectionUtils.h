#pragma once

#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <random>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <bitset>
#include "Sequence.h"
#include "HeteroPair.h"
#include "Numbers.h"
#include "RandomNumberGenerator.h"
#include "Composition.h"

class CollectionUtils {
public:
    template <typename A>
    static std::vector<int> getPermutationFromDisjointCycles(const std::set<std::vector<A>>& cs);

    static std::function<int(int)> getPermutationFunction(const std::vector<int>& permutation);

    static long getPermutationOrder(const std::vector<int>& permutation);

    static std::set<Sequence> getPermutationAsDisjointCycles(const std::vector<int>& permutation);

    template <typename A, typename B>
    static std::set<HeteroPair<A, B>> cartesianProduct(const std::set<A>& a, const std::set<B>& b);

    template <typename A, typename B>
    static std::vector<HeteroPair<A, B>> cartesianProduct(const std::vector<A>& a, const std::vector<B>& b);

    template <typename T, typename Predicate>
    static void filter(std::set<T>& s, Predicate p);

    template <typename T>
    static T chooseAtRandom(typename std::vector<T>::iterator i, int n);

    template <typename T>
    static T chooseAtRandomWithWeights(typename std::vector<T>::iterator i, int n, const std::vector<double>& weights);

    template <typename T>
    static T chooseAtRandom(const std::vector<T>& t);

    template <typename T, typename Equivalence>
    static std::vector<std::set<T>> partition(const std::set<T>& s, Equivalence e);

    template <typename T, typename U>
    static bool mapIsBijective(const std::map<T, U>& m);

    template <typename T>
    static std::vector<T> rotate(const std::vector<T>& arr, int n);

    template <typename T>
    static int countkins(const T& k, const std::vector<T>& a);

    template <typename T>
    static bool arrayEquals(const std::vector<T>& a, const std::vector<T>& b);

    static std::vector<int> mapUsingPermutation(const std::vector<int>& s, const std::vector<int>& p);

    template <typename U, typename T>
    static std::map<U, T> invertMap(const std::map<T, U>& map);

    static std::vector<int> map(const std::vector<int>& s, const std::map<int, int>& i);

    static int sizeOfCodomain(const std::vector<int>& s);

    static Sequence calcIntervalVector(const std::vector<bool>& input);

    static Sequence calcIntervalVector(const std::bitset<128>& input, int n);

    static std::map<int, Sequence> calcIntervalVector(const std::vector<int>& input);

    static std::vector<int> antidifference(const std::vector<int>& p_arr, int k);

    static std::vector<int> difference(const std::vector<int>& p_arr);

    static std::vector<int> cyclicalDifference(const std::vector<int>& p_arr);

    static std::vector<int> cyclicalAntidifference(const std::vector<int>& p_arr, int k);

    static std::vector<int> reverse(const std::vector<int>& p_arr);

    template <typename T>
    static std::vector<T> reverse(const std::vector<T>& arr);

    static std::string segmentationString(const std::vector<double>& cs, const Composition& p);

    static std::vector<int> randomPermutation(int n);

    template <typename T>
    static std::vector<T> permutate(const std::vector<int>& p, const std::vector<T>& arr);

    template <typename T>
    static std::vector<T> permutateRandomly(const std::vector<T>& arr);
};

template<typename A>
std::vector<int> CollectionUtils::getPermutationFromDisjointCycles(const std::set<std::vector<A>>& cs) {
    std::set<int> t;
    int sz = 0;

    for (const auto& c : cs) {
        sz += c.size();
        t.insert(c.begin(), c.end());
    }

    if (sz != (int)t.size() || *t.begin() != 0 || *t.rbegin() != sz - 1) {
        throw std::runtime_error("CollectionUtils::getPermutationFromDisjointCycles - invalid argument.");
    }

    std::vector<int> o(sz);

    for (const auto& c : cs) {
        for (size_t i = 0; i < c.size(); ++i) {
            o[c[i]] = c[(i + 1) % c.size()];
        }
    }
    return o;
}

template<typename A, typename B>
std::set<HeteroPair<A, B>> CollectionUtils::cartesianProduct(const std::set<A>& a, const std::set<B>& b) {
    std::set<HeteroPair<A, B>> o;
    for (const auto& x : a) {
        for (const auto& y : b) {
            o.insert(HeteroPair<A, B>::makeHeteroPair(x, y));
        }
    }
    return o;
}

template<typename A, typename B>
std::vector<HeteroPair<A, B>> CollectionUtils::cartesianProduct(const std::vector<A>& a, const std::vector<B>& b) {
    std::vector<HeteroPair<A, B>> o;
    for (const auto& x : a) {
        for (const auto& y : b) {
            o.push_back(HeteroPair<A, B>::makeHeteroPair(x, y));
        }
    }
    return o;
}

template<typename T, typename Predicate>
void CollectionUtils::filter(std::set<T>& s, Predicate p) {
    std::vector<T> x;
    for (const auto& a : s) {
        if (p(a)) {
            x.push_back(a);
        }
    }
    s.clear();
    s.insert(x.begin(), x.end());
}

template<typename T>
T CollectionUtils::chooseAtRandom(typename std::vector<T>::iterator i, int n) {
    if (n <= 0) {
        throw std::runtime_error("CollectionUtils::chooseAtRandom Invalid arguments");
    }

    T o;
    int k = 0;
    int r = RandomNumberGenerator::nextInt(n);

    while (k <= r) {
        o = *i;
        ++i;
        ++k;
    }

    return o;
}

template<typename T>
bool CollectionUtils::arrayEquals(const std::vector<T>& a, const std::vector<T>& b) {
    return a == b;
}

template<typename U, typename T>
std::map<U, T> CollectionUtils::invertMap(const std::map<T, U>& map) {
    if (!mapIsBijective(map)) {
        throw std::runtime_error("The map is not bijective; it cannot be inverted.");
    }
    std::map<U, T> o;
    for (const auto& e : map) {
        o[e.second] = e.first;
    }
    return o;
}

template<typename T>
std::vector<T> CollectionUtils::reverse(const std::vector<T>& arr) {
    std::vector<T> o(arr.rbegin(), arr.rend());
    return o;
}

template<typename T>
std::vector<T> CollectionUtils::permutate(const std::vector<int>& p, const std::vector<T>& arr) {
    std::vector<T> o(arr.size());
    for (size_t i = 0; i < arr.size(); ++i) {
        o[i] = arr[p[i]];
    }
    return o;
}

template<typename T>
std::vector<T> CollectionUtils::permutateRandomly(const std::vector<T>& arr) {
    return permutate(randomPermutation(arr.size()), arr);
}

template<typename T>
T CollectionUtils::chooseAtRandom(const std::vector<T>& t) {
    if (t.empty()) return T();
    return t[RandomNumberGenerator::nextInt(t.size())];
}

template<typename T, typename Equivalence>
std::vector<std::set<T>> CollectionUtils::partition(const std::set<T>& s, Equivalence e) {
    std::vector<std::set<T>> o;
    std::vector<T> t(s.begin(), s.end());

    while (!t.empty()) {
        T elem = t.front();
        t.erase(t.begin());

        bool found = false;

        for (auto& i : o) {
            if (e(i.begin(), elem)) {
                i.insert(elem);
                found = true;
                break;
            }
        }

        if (!found) {
            std::set<T> n;
            n.insert(elem);
            o.push_back(n);
        }
    }

    return o;
}

template<typename T, typename U>
bool CollectionUtils::mapIsBijective(const std::map<T, U>& m) {
    std::set<U> values(m.begin(), m.end());
    return m.size() == values.size();
}

template<typename T>
std::vector<T> CollectionUtils::rotate(const std::vector<T>& arr, int n) {
    std::vector<T> c(arr);

    int m = n;

    while (m < 0) {
        m += c.size();
    }
    while (m > (int)c.size()) {
        m -= c.size();
    }

    std::rotate(c.rbegin(), c.rbegin() + m, c.rend());

    return c;
}

template<typename T>
int CollectionUtils::countkins(const T& k, const std::vector<T>& a) {
    return std::count(a.begin(), a.end(), k);
}
