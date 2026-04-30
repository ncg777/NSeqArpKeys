#include "CollectionUtils.h"

std::function<int(int)> CollectionUtils::getPermutationFunction(const std::vector<int>& permutation) {
    return [permutation](int i) { return permutation[i]; };
}

long CollectionUtils::getPermutationOrder(const std::vector<int>& permutation) {
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

std::set<Sequence> CollectionUtils::getPermutationAsDisjointCycles(const std::vector<int>& permutation) {
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

std::vector<int> CollectionUtils::mapUsingPermutation(const std::vector<int>& s, const std::vector<int>& p) {
    std::map<int, int> i;
    for (size_t x = 0; x < p.size(); ++x) {
        i[x] = p[x];
    }
    return map(s, i);
}

std::vector<int> CollectionUtils::map(const std::vector<int>& s, const std::map<int, int>& i) {
    std::vector<int> output(s.size());
    for (size_t x = 0; x < s.size(); ++x) {
        output[x] = i.count(s[x]) ? i.at(s[x]) : s[x];
    }
    return output;
}

int CollectionUtils::sizeOfCodomain(const std::vector<int>& s) {
    std::set<int> t(s.begin(), s.end());
    return t.size();
}

Sequence CollectionUtils::calcIntervalVector(const std::bitset<128>& input, int n) {
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

std::map<int, Sequence> CollectionUtils::calcIntervalVector(const std::vector<int>& input) {
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

std::vector<int> CollectionUtils::antidifference(const std::vector<int>& p_arr, int k) {
    std::vector<int> output(p_arr.size() + 1);

    output[0] = k;

    for (size_t i = 0; i < p_arr.size(); ++i) {
        output[i + 1] = output[i] + p_arr[i];
    }
    return output;
}

std::vector<int> CollectionUtils::difference(const std::vector<int>& p_arr) {
    std::vector<int> output(p_arr.size() - 1);

    for (size_t i = 1; i < p_arr.size(); ++i) {
        output[i - 1] = p_arr[i] - p_arr[i - 1];
    }
    return output;
}

std::vector<int> CollectionUtils::cyclicalDifference(const std::vector<int>& p_arr) {
    std::vector<int> output(p_arr.size());

    for (size_t i = 0; i < p_arr.size(); ++i) {
        output[i % p_arr.size()] = p_arr[(i + 1) % p_arr.size()] - p_arr[i];
    }
    return output;
}

std::vector<int> CollectionUtils::cyclicalAntidifference(const std::vector<int>& p_arr, int k) {
    std::vector<int> output(p_arr.size());

    output[p_arr.size() - 1] = k;

    for (size_t i = 0; i < p_arr.size(); ++i) {
        output[i] = output[(i - 1 + p_arr.size()) % p_arr.size()] + p_arr[(i - 1 + p_arr.size()) % p_arr.size()];
    }

    return output;
}

std::string CollectionUtils::segmentationString(const std::vector<double>& cs, const Composition& p) {
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

std::vector<int> CollectionUtils::randomPermutation(int n) {
    std::vector<int> o(n);
    std::iota(o.begin(), o.end(), 0);
    std::shuffle(o.begin(), o.end(), std::mt19937{ std::random_device{}() });
    return o;
}

std::vector<int> CollectionUtils::reverse(const std::vector<int>& p_arr) {
    std::vector<int> output(p_arr.rbegin(), p_arr.rend());
    return output;
}

Sequence CollectionUtils::calcIntervalVector(const std::vector<bool>& input) {
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
