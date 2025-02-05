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
#include "CombinationEnumeration.h"
class Combination : public std::bitset<128> {
public:
    Combination(const Combination& c) : std::bitset<128>(c), m_n(c.m_n) {}
    Combination(int n) : m_n(n) {}
    static int find_first(const std::bitset<128>& bitset, int n) {
        for (int i = 0; i < n; ++i) {
            if (bitset.test(i)) {
                return i;
            }
        }
        return -1;
    }

    static int find_next(const std::bitset<128>& bitset, int pos, int n) {
        for (int i = pos + 1; i < n; ++i) {
            if (bitset.test(i)) {
                return i;
            }
        }
        return -1;
    }

    static int find_last(const std::bitset<128>& bitset, int n) {
        for (int i = n - 1; i >= 0; --i) {
            if (bitset.test(i)) {
                return i;
            }
        }
        return -1;
    }

    int find_first() const {
        for (int i = 0; i < m_n; ++i) {
            if (this->test(i)) {
                return i;
            }
        }
        return -1;
    }

    int find_next(int pos) const {
        for (int i = pos + 1; i < m_n; ++i) {
            if (this->test(i)) {
                return i;
            }
        }
        return -1;
    }
    int find_last() const {
        for (int i = m_n - 1; i >= 0; --i) {
            if (this->test(i)) {
                return i;
            }
        }
        return -1;
    }
    int getN() const {
        return m_n;
    }

    int getK() const {
        return this->count();
    }

    int calcSpan() const {
        return getN() - Composition::getCompositionFromCombination(*this).asSequence().getMax();
    }

    Sequence getIntervalVector() const {
        return CollectionUtils::calcIntervalVector(*this, m_n);
    }

    Combination(int n, const std::set<int>& s) : Combination(n) {
        for (int i = 0; i < n; ++i) {
            this->set(i, s.find(i) != s.end());
        }
    }

    Combination(const Combination& c) : Combination(c.m_n) {
        this->operator|=(c);
    }

    Combination(const std::bitset<128>& c, int n) : std::bitset<128>(c), m_n(n) {}


    Combination reverse() const {
        Combination o(this->getN());
        for (int i = 0; i < o.getN(); ++i) {
            if (this->test(i)) o.set(-1 + o.getN() - i);
        }
        return o;
    }

    double calcNormalizedDistanceWith(const Combination& other) const {
        int maxn = std::max(this->getN(), other.getN());
        int acc = 0;
        for (int i = 0; i < maxn; ++i) {
            if (this->test(i % this->getN()) != other.test(i % other.getN())) acc++;
        }
        return static_cast<double>(acc) / static_cast<double>(maxn);
    }

    Combination symmetricDifference(const Combination& y) const {
        int n = std::max(this->getN(), y.getN());
        Combination x = Combination(n);
        x |= *this;
        x ^= y;
        
        return x;
    }

    std::string toBinaryString() const {
        std::ostringstream oss;
        for (int i = 0; i < m_n; ++i) {
            oss << (this->test(i) ? "1" : "0");
        }
        return oss.str();
    }

    static Combination fromBinaryString(const std::string& s) {
        std::string trimmed = s;
        trimmed.erase(trimmed.find_last_not_of(" \n\r\t") + 1);
        int n = trimmed.length();
        Combination o(n);
        for (int i = 0; i < n; ++i) {
            if (trimmed[i] != '0') o.set(i);
        }
        return o;
    }

    static Combination mergeAll(const std::vector<Combination>& r) {
        Sequence sizes;
        for (const auto& comb : r) {
            sizes.push_back(comb.getN());
        }
        int max = sizes.getMax();
        int newsz = max * r.size();
        Combination b(newsz);
        Sequence idxs(r.size(), 0);
        for (int i = 0; i < newsz; ++i) {
            int wi = i % r.size();
            const auto& w = r[wi];
            b.set(i, w.test(idxs[wi]));
            idxs[wi] = (idxs[wi] + 1) % sizes[wi];
        }
        return b;
    }

    Sequence homogeneityRegionsSequence() const {
        Sequence s = Composition::getCompositionFromCombination(*this).asSequence();
        Sequence groups(s.size(), 0);
        int k = 0;
        for (int j = s.size() - 1; j >= 0; --j) {
            if (s[0] == s[j]) {
                k--;
            }
            else {
                break;
            }
        }
        k += s.size();
        k = k % s.size();
        int previousValue = s[k];
        int currentGroup = 0;
        for (int i = k + 1; i < s.size() + k; ++i) {
            int v = s[i % s.size()];
            if (v != previousValue) {
                currentGroup++;
            }
            groups[i % groups.size()] = currentGroup;
            previousValue = v;
        }
        return groups;
    }

    std::vector<Combination> decomposeIntoHomogeneousRegions() const {
        std::vector<Combination> o;
        Sequence seq = Composition::getCompositionFromCombination(*this).asSequence();
        Sequence partition = this->homogeneityRegionsSequence();
        Sequence deduped;
        int last = seq[0];
        deduped.push_back(last);
        for (const auto& i : seq) {
            if (i != last) {
                deduped.push_back(i);
                last = i;
            }
        }
        int n = partition.getMax() + 1;
        int k = this->find_first();
        int j = 0;
        for (int i = 0; i < n; ++i) {
            Combination comb(this->getN());
            int val = deduped[i];
            while (true) {
                comb.set(k);
                k += val;
                if (++j == seq.size() || seq[j] != val) {
                    break;
                }
            }
            o.push_back(comb);
        }
        return o;
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << "{";
        for (int i = this->find_first(); i >= 0; i = this->find_next(i)) {
            oss << i;
            if (i != this->find_last()) {
                oss << ", ";
            }
        }
        oss << "}";
        return oss.str();
    }

    Sequence asSequence() const {
        Sequence o;
        for (int i = this->find_first(); i >= 0; i = this->find_next(i + 1)) {
            o.push_back(i);
        }
        return o;
    }

    std::set<int> asSet() const {
        std::set<int> o;
        for (int i = this->find_first(); i >= 0; i = this->find_next(i + 1)) {
            o.insert(i);
        }
        return o;
    }

    Sequence asBinarySequence() const {
        Sequence o;
        for (int i = 0; i < getN(); ++i) {
            o.push_back(this->test(i) ? 1 : 0);
        }
        return o;
    }

    static Combination fromBinarySequence(const Sequence& s) {
        Combination c(s.size());
        for (int i = 0; i < s.size(); ++i) {
            if (s[i] != 0) c.set(i, true);
        }
        return c;
    }

    int compareTo(const Combination& o) const {
        if (this->m_n < o.m_n) {
            return -1;
        }
        if (this->m_n > o.m_n) {
            return 1;
        }
        std::bitset<128> a;
        a |= *this;
        std::bitset<128> b;
        b |= o;
        a ^= b;
        int i = find_first(a, this->m_n);
        if (i == -1) {
            return 0;
        }
        else {
            return b.test(i) ? -1 : 1;
        }
    }

    static std::vector<Combination> refinements(const Combination& c) {
        int n = c.getN() - c.getK();
        if (n == 0) {
            return {};
        }
        std::vector<Combination> o;
        int k = 0;
        for (int i = 0; i < n; ++i) {
            while (c.test(k)) {
                k++;
            }
            Combination b = Combination(c.getN());
            b |= c;
            b.set(k++);
            o.push_back(b);
        }
        return o;
    }

    static Combination merge(const Combination& a, const Combination& b) {
        Combination x = Combination(std::max(a.getN(), b.getN()));
        x |= a;
        x |= b;
        return x;
    }

    static std::vector<Combination> generate(int n, int k) {
        int cnt = static_cast<int>(Numbers::binomial(n, k));
        std::vector<Combination> o(cnt);
        CombinationEnumeration ce(n, k);
        int i = 0;
        while (i < cnt) {
            o[i++] = ce.nextElement();
        }
        return o;
    }

    Combination rotate(int t) const {
        int k = -t;
        while (k < 0) {
            k += m_n;
        }
        while (k >= m_n) {
            k -= m_n;
        }
        Combination x = Combination(this->getN());
        for (int i = 0; i < m_n; ++i) {
            x.set(i, this->test((i - k + m_n) % m_n));
        }
        return x;
    }

    Combination intersect(const Combination& c) const {
        int n = std::min(this->getN(), c.getN());
        Combination b = Combination(n);
        b |= *this;
        b &= c;
        return b;
    }

    Combination minus(const Combination& c) const {
        Combination o(*this);
        int n = std::min(getN(), c.getN());
        for (int i = 0; i < n; ++i) {
            if (o.test(i) && c.test(i)) {
                o.set(i, false);
            }
        }
        return o;
    }

    template <typename T>
    std::vector<T> applyTo(const std::vector<T>& arr) const {
        if (arr.size() != this->getN()) {
            throw std::invalid_argument("Array size does not match combination size");
        }
        std::vector<T> o(this->getK());
        int k = 0;
        for (int i = this->find_first(); i > -1; i = this->find_next(i + 1)) {
            o[k++] = arr[i];
        }
        return o;
    }

    Sequence applyTo(const Sequence& seq) const {
        return Sequence(this->applyTo(seq));
    }

    std::vector<Combination> partition(const Sequence& p0) const {
        std::vector<int> p(p0.begin(), p0.end());
        return partition(p);
    }

    std::vector<Combination> partition(const std::vector<int>& partition) const {
        if (partition.size() != this->getK()) {
            throw std::invalid_argument("Partition size does not match combination size");
        }
        int min = *std::min_element(partition.begin(), partition.end());
        int max = *std::max_element(partition.begin(), partition.end());
        if (min != 0 || max > getK()) {
            throw std::invalid_argument("Invalid partition values");
        }
        std::vector<int> set(this->getK());
        int k = 0;
        for (int i = this->find_first(); i > -1; i = this->find_next(i + 1)) {
            set[k++] = i;
        }
        std::vector<Combination> o(max + 1);
        std::vector<Combination> b(max + 1);
        for (int i = 0; i <= max; ++i) {
            b[i] = Combination(getN());
            for (int j = 0; j < partition.size(); ++j) {
                if (partition[j] == i) {
                    b[i].set(set[j]);
                }
            }
            o[i] = Combination(b[i]);
        }
        return o;
    }


    static Combination genRnd(int n) {
        return genRnd(n, RandomNumberGenerator::nextInt(n + 1));
    }

    static Combination genRnd(int n, int k) {
        Combination o(n);
        int c = 0;
        int i = 0;
        double d;
        while (c < k) {
            d = RandomNumberGenerator::nextDouble();
            if (d < static_cast<double>(k - c) / static_cast<double>(n - i)) {
                o.set(i, true);
                c++;
            }
            i++;
        }
        return o;
    }

private:
    int m_n;
};

