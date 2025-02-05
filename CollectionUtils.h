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
    static std::vector<int> getPermutationFromDisjointCycles(const std::set<std::vector<A>>& cs) {
        std::set<int> t;
        int sz = 0;

        for (const auto& c : cs) {
            sz += c.size();
            t.insert(c.begin(), c.end());
        }

        if (sz != t.size() || *t.begin() != 0 || *t.rbegin() != sz - 1) {
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

    static std::function<int(int)> getPermutationFunction(const std::vector<int>& permutation) {
        return [permutation](int i) { return permutation[i]; };
    }

    static long getPermutationOrder(const std::vector<int>& permutation) {
        auto cs = getPermutationAsDisjointCycles(permutation);

        long o = 0;

        for (const auto& s : cs) {
            if (o == 0) {
                o = s.size();
            }
            else {
                o = Numbers::lcm(o, s.size());
            }
        }
        return o;
    }

    static std::set<Sequence> getPermutationAsDisjointCycles(const std::vector<int>& permutation) {
        std::set<Sequence> o;
        Sequence s(permutation.begin(), permutation.end());

        auto f = getPermutationFunction(permutation);

        while (!s.empty()) {
            Sequence c;

            int initial = s[0];

            int current = initial;

            do {
                c.push_back(current);
                current = f(current);
            } while (current != initial);
            s.erase(std::remove(s.begin(), s.end(), initial), s.end());
            o.insert(c);
        }
        return o;
    }

    template <typename A, typename B>
    static std::set<HeteroPair<A, B>> cartesianProduct(const std::set<A>& a, const std::set<B>& b) {
        std::set<HeteroPair<A, B>> o;
        for (const auto& x : a) {
            for (const auto& y : b) {
                o.insert(HeteroPair<A, B>::makeHeteroPair(x, y));
            }
        }
        return o;
    }

    template <typename A, typename B>
    static std::vector<HeteroPair<A, B>> cartesianProduct(const std::vector<A>& a, const std::vector<B>& b) {
        std::vector<HeteroPair<A, B>> o;
        for (const auto& x : a) {
            for (const auto& y : b) {
                o.push_back(HeteroPair<A, B>::makeHeteroPair(x, y));
            }
        }
        return o;
    }

    static Matrix<int> enumerate(int n, int range) {
        if (n < 1 || range == 0) {
            throw std::runtime_error("CollectionUtils::enumerate Invalid arguments");
        }
        int sign = (range > 0) ? 1 : -1;
        if (n == 1) {
            Matrix<int> m0(std::abs(range), 1, 0);
            for (int i = 0; i < std::abs(range); ++i) {
                m0.set(i, 0, i * sign);
            }
            return m0;
        }
        else {
            Matrix<int> m0 = enumerate(n - 1, range);
            int nbRowM0 = m0.rowCount();

            int mr = std::abs(range) * m0.rowCount();
            int nr = m0.columnCount() + 1;

            Matrix<int> m1(mr, nr, 0);

            std::vector<int> cz(std::abs(range) * m0.rowCount());
            for (int i = 0; i < std::abs(range); ++i) {
                m1.setBlock(m0, i * nbRowM0, 0);
                for (int j = 0; j < m0.rowCount(); ++j) {
                    cz.push_back(i * sign);
                }
            }
            m1.setColumn(nr - 1, cz);
            return m1;
        }
    }

    template <typename T, typename Predicate>
    static void filter(std::set<T>& s, Predicate p) {
        std::vector<T> x;
        for (const auto& a : s) {
            if (p(a)) {
                x.push_back(a);
            }
        }
        s.clear();
        s.insert(x.begin(), x.end());
    }

    template <typename T>
    static T chooseAtRandom(typename std::vector<T>::iterator i, int n) {
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

    template <typename T>
    static T chooseAtRandomWithWeights(typename std::vector<T>::iterator i, int n, const std::vector<double>& weights) {
        if (n <= 0 || weights.size() != n) {
            throw std::runtime_error("CollectionUtils::chooseAtRandom Invalid arguments");
        }

        std::vector<double> normalizedWeights(n);
        double sum = std::accumulate(weights.begin(), weights.end(), 0.0);
        double sum_normalized = 0.0;
        for (int j = 0; j < n; ++j) {
            if (j == n - 1) {
                normalizedWeights[j] = 1.0 - sum_normalized;
            }
            else {
                normalizedWeights[j] = weights[j] / sum;
                sum_normalized += normalizedWeights[j];
            }
        }

        double acc = 0.0;

        T o;
        int k = 0;
        double r = RandomNumberGenerator::nextDouble();

        while (i != i.end()) {
            o = *i;
            acc += normalizedWeights[k];
            if (r < acc) break;
            ++i;
            ++k;
        }

        return o;
    }

    template <typename T>
    static T chooseAtRandom(const std::vector<T>& t) {
        return chooseAtRandom(t.begin(), t.size());
    }

    template <typename T>
    static T chooseAtRandomWithWeights(const std::vector<T>& t, const std::vector<double>& weights) {
        return chooseAtRandomWithWeights(t.begin(), t.size(), weights);
    }

    template <typename T>
    static T chooseAtRandom(const std::vector<T>& t) {
        if (t.empty()) return T();
        return t[RandomNumberGenerator::nextInt(t.size())];
    }

    template <typename T, typename Equivalence>
    static std::vector<std::set<T>> partition(const std::set<T>& s, Equivalence e) {
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

    template <typename T, typename U>
    static bool mapIsBijective(const std::map<T, U>& m) {
        std::set<U> values(m.begin(), m.end());
        return m.size() == values.size();
    }

    template <typename T>
    static std::vector<T> rotate(const std::vector<T>& arr, int n) {
        std::vector<T> c(arr);

        int m = n;

        while (m < 0) {
            m += c.size();
        }
        while (m > c.size()) {
            m -= c.size();
        }

        std::rotate(c.rbegin(), c.rbegin() + m, c.rend());

        return c;
    }

    template <typename T>
    static int countkins(const T& k, const std::vector<T>& a) {
        return std::count(a.begin(), a.end(), k);
    }

    template <typename T>
    static bool arrayEquals(const std::vector<T>& a, const std::vector<T>& b) {
        return a == b;
    }

    static std::vector<int> mapUsingPermutation(const std::vector<int>& s, const std::vector<int>& p) {
        std::map<int, int> i;
        for (size_t x = 0; x < p.size(); ++x) {
            i[x] = p[x];
        }
        return map(s, i);
    }

    template <typename U, typename T>
    static std::map<U, T> invertMap(const std::map<T, U>& map) {
        if (!mapIsBijective(map)) {
            throw std::runtime_error("The map is not bijective; it cannot be inverted.");
        }
        std::map<U, T> o;
        for (const auto& e : map) {
            o[e.second] = e.first;
        }
        return o;
    }

    static std::vector<int> map(const std::vector<int>& s, const std::map<int, int>& i) {
        std::vector<int> output(s.size());
        for (size_t x = 0; x < s.size(); ++x) {
            output[x] = i.count(s[x]) ? i.at(s[x]) : s[x];
        }
        return output;
    }

    static int sizeOfCodomain(const std::vector<int>& s) {
        std::set<int> t(s.begin(), s.end());
        return t.size();
    }

    static Sequence calcIntervalVector(const std::vector<bool>& input) {
        int n = input.size();
        int m = n / 2;
        Sequence s;

        for (int i = 1; i <= m; ++i) {
            int k = 0;
            for (int j = 0; j < n; ++j) {
                if (input[j] && input[(i + j) % n]) {
                    k++;
                }
            }
            if (i == m && n % 2 == 0) {
                k /= 2;
            }
            s.push_back(k);
        }
        return s;
    }

    static Sequence calcIntervalVector(const std::bitset<128>& input, int n) {
        int m = n / 2;
        Sequence s;

        for (int i = 1; i <= m; ++i) {
            int k = 0;
            for (int j = 0; j < n; ++j) {
                if (input[j] && input[(i + j) % n]) {
                    k++;
                }
            }
            if (i == m && n % 2 == 0) {
                k /= 2;
            }
            s.push_back(k);
        }
        return s;
    }

    static std::map<int, Sequence> calcIntervalVector(const Sequence& input) {
        std::map<int, Sequence> output;
        std::set<int> t(input.begin(), input.end());

        for (int v : t) {
            std::vector<bool> b(input.size());
            for (size_t j = 0; j < input.size(); ++j) {
                b[j] = input[j] == v;
            }
            output[v] = calcIntervalVector(b);
        }
        return output;
    }

    static std::map<int, Sequence> calcIntervalVector(const std::vector<int>& input) {
        Sequence s(input.begin(), input.end());
        return calcIntervalVector(s);
    }

    static std::vector<int> antidifference(const std::vector<int>& p_arr, int k) {
        std::vector<int> output(p_arr.size() + 1);

        output[0] = k;

        for (size_t i = 0; i < p_arr.size(); ++i) {
            output[i + 1] = output[i] + p_arr[i];
        }
        return output;
    }

    static std::vector<int> difference(const std::vector<int>& p_arr) {
        std::vector<int> output(p_arr.size() - 1);

        for (size_t i = 1; i < p_arr.size(); ++i) {
            output[i - 1] = p_arr[i] - p_arr[i - 1];
        }
        return output;
    }

    static std::vector<int> cyclicalDifference(const std::vector<int>& p_arr) {
        std::vector<int> output(p_arr.size());

        for (size_t i = 0; i < p_arr.size(); ++i) {
            output[i % p_arr.size()] = p_arr[(i + 1) % p_arr.size()] - p_arr[i];
        }
        return output;
    }

    static std::vector<int> cyclicalAntidifference(const std::vector<int>& p_arr, int k) {
        std::vector<int> output(p_arr.size());

        output[p_arr.size() - 1] = k;

        for (size_t i = 0; i < p_arr.size(); ++i) {
            output[i] = output[(i - 1 + p_arr.size()) % p_arr.size()] + p_arr[(i - 1 + p_arr.size()) % p_arr.size()];
        }

        return output;
    }

    static std::vector<int> reverse(const std::vector<int>& p_arr) {
        std::vector<int> output(p_arr.rbegin(), p_arr.rend());
        return output;
    }

    template <typename T>
    static std::vector<T> reverse(const std::vector<T>& arr) {
        std::vector<T> o(arr.rbegin(), arr.rend());
        return o;
    }

    static std::string segmentationString(const std::vector<double>& cs, const Composition& p) {
        if (p.getTotal() != cs.size()) {
            throw std::invalid_argument("Invalid argument");
        }

        std::ostringstream oss;

        Sequence ps = p.asSequence();
        int k = ps.size();
        int offset = 0;

        for (int i = 0; i < k; ++i) {
            oss << "(";
            int x = ps[i];
            for (int j = 0; j < x; ++j) {
                oss << cs[offset + j];
                if (j < x - 1) {
                    oss << " ";
                }
            }
            oss << ")";
            if (i < k - 1) {
                oss << ",\n";
            }
            offset += ps[i];
        }
        return oss.str();
    }

    static std::vector<int> randomPermutation(int n) {
        std::vector<int> o(n);
        std::iota(o.begin(), o.end(), 0);
        std::shuffle(o.begin(), o.end(), std::mt19937{ std::random_device{}() });
        return o;
    }

    template <typename T>
    static std::vector<T> permutate(const std::vector<int>& p, const std::vector<T>& arr) {
        std::vector<T> o(arr.size());
        for (size_t i = 0; i < arr.size(); ++i) {
            o[i] = arr[p[i]];
        }
        return o;
    }

    template <typename T>
    static std::vector<T> permutateRandomly(const std::vector<T>& arr) {
        return permutate(randomPermutation(arr.size()), arr);
    }
};



