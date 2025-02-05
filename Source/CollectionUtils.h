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
#include "HeteroPair.h"
#include "Matrix.h"
#include "Numbers.h"
#include "Sequence.h"
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

    static std::map<int, Sequence> calcIntervalVector(const Sequence& input);

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

