#include "CollectionUtils.h"

inline std::function<int(int)> CollectionUtils::getPermutationFunction(const std::vector<int>& permutation) {
    return [permutation](int i) { return permutation[i]; };
}

inline long CollectionUtils::getPermutationOrder(const std::vector<int>& permutation) {
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

inline std::set<Sequence> CollectionUtils::getPermutationAsDisjointCycles(const std::vector<int>& permutation) {
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

inline std::vector<int> CollectionUtils::mapUsingPermutation(const std::vector<int>& s, const std::vector<int>& p) {
    std::map<int, int> i;
    for (size_t x = 0; x < p.size(); ++x) {
        i[x] = p[x];
    }
    return map(s, i);
}

inline std::vector<int> CollectionUtils::map(const std::vector<int>& s, const std::map<int, int>& i) {
    std::vector<int> output(s.size());
    for (size_t x = 0; x < s.size(); ++x) {
        output[x] = i.count(s[x]) ? i.at(s[x]) : s[x];
    }
    return output;
}

inline int CollectionUtils::sizeOfCodomain(const std::vector<int>& s) {
    std::set<int> t(s.begin(), s.end());
    return t.size();
}

inline Sequence CollectionUtils::calcIntervalVector(const std::bitset<128>& input, int n) {
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

inline std::map<int, Sequence> CollectionUtils::calcIntervalVector(const Sequence& input) {
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

inline std::map<int, Sequence> CollectionUtils::calcIntervalVector(const std::vector<int>& input) {
    Sequence s(input.begin(), input.end());
    return calcIntervalVector(s);
}

inline std::vector<int> CollectionUtils::antidifference(const std::vector<int>& p_arr, int k) {
    std::vector<int> output(p_arr.size() + 1);

    output[0] = k;

    for (size_t i = 0; i < p_arr.size(); ++i) {
        output[i + 1] = output[i] + p_arr[i];
    }
    return output;
}

inline std::vector<int> CollectionUtils::difference(const std::vector<int>& p_arr) {
    std::vector<int> output(p_arr.size() - 1);

    for (size_t i = 1; i < p_arr.size(); ++i) {
        output[i - 1] = p_arr[i] - p_arr[i - 1];
    }
    return output;
}

inline std::vector<int> CollectionUtils::cyclicalDifference(const std::vector<int>& p_arr) {
    std::vector<int> output(p_arr.size());

    for (size_t i = 0; i < p_arr.size(); ++i) {
        output[i % p_arr.size()] = p_arr[(i + 1) % p_arr.size()] - p_arr[i];
    }
    return output;
}

inline std::vector<int> CollectionUtils::cyclicalAntidifference(const std::vector<int>& p_arr, int k) {
    std::vector<int> output(p_arr.size());

    output[p_arr.size() - 1] = k;

    for (size_t i = 0; i < p_arr.size(); ++i) {
        output[i] = output[(i - 1 + p_arr.size()) % p_arr.size()] + p_arr[(i - 1 + p_arr.size()) % p_arr.size()];
    }

    return output;
}

inline std::string CollectionUtils::segmentationString(const std::vector<double>& cs, const Composition& p) {
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

inline std::vector<int> CollectionUtils::randomPermutation(int n) {
    std::vector<int> o(n);
    std::iota(o.begin(), o.end(), 0);
    std::shuffle(o.begin(), o.end(), std::mt19937{ std::random_device{}() });
    return o;
}

inline std::vector<int> CollectionUtils::reverse(const std::vector<int>& p_arr) {
    std::vector<int> output(p_arr.rbegin(), p_arr.rend());
    return output;
}

inline Sequence CollectionUtils::calcIntervalVector(const std::vector<bool>& input) {
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

template<typename A>
inline std::vector<int> CollectionUtils::getPermutationFromDisjointCycles(const std::set<std::vector<A>>& cs) {
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

template<typename A, typename B>
inline std::set<HeteroPair<A, B>> CollectionUtils::cartesianProduct(const std::set<A>& a, const std::set<B>& b) {
    std::set<HeteroPair<A, B>> o;
    for (const auto& x : a) {
        for (const auto& y : b) {
            o.insert(HeteroPair<A, B>::makeHeteroPair(x, y));
        }
    }
    return o;
}

template<typename A, typename B>
inline std::vector<HeteroPair<A, B>> CollectionUtils::cartesianProduct(const std::vector<A>& a, const std::vector<B>& b) {
    std::vector<HeteroPair<A, B>> o;
    for (const auto& x : a) {
        for (const auto& y : b) {
            o.push_back(HeteroPair<A, B>::makeHeteroPair(x, y));
        }
    }
    return o;
}

template<typename T, typename Predicate>
inline void CollectionUtils::filter(std::set<T>& s, Predicate p) {
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
inline T CollectionUtils::chooseAtRandom(typename std::vector<T>::iterator i, int n) {
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
inline T CollectionUtils::chooseAtRandomWithWeights(typename std::vector<T>::iterator i, int n, const std::vector<double>& weights) {
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

template<typename T>
inline bool CollectionUtils::arrayEquals(const std::vector<T>& a, const std::vector<T>& b) {
    return a == b;
}

template<typename U, typename T>
inline std::map<U, T> CollectionUtils::invertMap(const std::map<T, U>& map) {
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
inline std::vector<T> CollectionUtils::reverse(const std::vector<T>& arr) {
    std::vector<T> o(arr.rbegin(), arr.rend());
    return o;
}

template<typename T>
inline std::vector<T> CollectionUtils::permutate(const std::vector<int>& p, const std::vector<T>& arr) {
    std::vector<T> o(arr.size());
    for (size_t i = 0; i < arr.size(); ++i) {
        o[i] = arr[p[i]];
    }
    return o;
}

template<typename T>
inline std::vector<T> CollectionUtils::permutateRandomly(const std::vector<T>& arr) {
    return permutate(randomPermutation(arr.size()), arr);
}

template<typename T>
inline T CollectionUtils::chooseAtRandom(const std::vector<T>& t) {
    if (t.empty()) return T();
    return t[RandomNumberGenerator::nextInt(t.size())];
}

template<typename T, typename Equivalence>
inline std::vector<std::set<T>> CollectionUtils::partition(const std::set<T>& s, Equivalence e) {
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
inline bool CollectionUtils::mapIsBijective(const std::map<T, U>& m) {
    std::set<U> values(m.begin(), m.end());
    return m.size() == values.size();
}

template<typename T>
inline std::vector<T> CollectionUtils::rotate(const std::vector<T>& arr, int n) {
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

template<typename T>
inline int CollectionUtils::countkins(const T& k, const std::vector<T>& a) {
    return std::count(a.begin(), a.end(), k);
}

template<typename T>
inline T CollectionUtils::chooseAtRandom(const std::vector<T>& t) {
    return chooseAtRandom(t.begin(), t.size());
}
