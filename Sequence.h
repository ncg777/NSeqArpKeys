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
#include "CollectionUtils.h"
#include "HomoPair.h"
#include "Matrix.h"
#include "Numbers.h"
#include "RandomNumberGenerator.h"

class Sequence : public std::vector<int>, public std::function<int(int)> {
public:
    using std::vector<int>::vector;

    bool isNatural() const {
        return std::all_of(this->begin(), this->end(), [](int n) { return n >= 0; });
    }
    Sequence(const std::vector<int>& p_arr) : std::vector<int>(p_arr) {}
    std::vector<double> getSymmetries() const {
        std::vector<double> o;
        int n = this->size();
        for (int i = 0; i < n * 2; ++i) {
            int axis = i / 2;
            bool found = true;
            if (i % 2 == 0) {
                for (int j = 0; j < (1 + (n / 2)); ++j) {
                    if ((*this)[(axis + j) % n] != (*this)[(n + axis - j) % n]) {
                        found = false;
                        break;
                    }
                }
            }
            else {
                for (int j = 0; j < (1 + (n / 2)); ++j) {
                    if ((*this)[(axis + j + 1) % n] != (*this)[(n + axis - j) % n]) {
                        found = false;
                        break;
                    }
                }
            }
            if (found) {
                o.push_back(static_cast<double>(i) / 2.0);
            }
        }
        return o;
    }

    enum class ArpType {
        UP,
        DOWN,
        UPDOWN,
        DOWNUP,
        SIGMAUP,
        SIGMADOWN
    };

    static Sequence arp(ArpType arpType, int k, bool repeatBottom = false, bool repeatTop = false) {
        if (k < 2) {
            throw std::runtime_error("k<2");
        }
        Sequence o;
        switch (arpType) {
        case ArpType::UP:
            o = o.juxtapose(Sequence::stair(0, k, 1));
            if (repeatBottom) {
                o.insert(o.begin(), 0);
            }
            if (repeatTop) {
                o.push_back(k - 1);
            }
            break;
        case ArpType::DOWN:
            o = o.juxtapose(Sequence::stair(k - 1, k, -1));
            if (repeatBottom) {
                o.push_back(0);
            }
            if (repeatTop) {
                o.insert(o.begin(), k - 1);
            }
            break;
        case ArpType::UPDOWN:
            o = o.juxtapose(Sequence::stair(0, k, 1));
            if (repeatBottom) {
                o.insert(o.begin(), 0);
            }
            if (repeatTop) {
                o.push_back(k - 1);
            }
            o = o.juxtapose(Sequence::stair(k - 2, k - 2, -1));
            break;
        case ArpType::DOWNUP:
            o = o.juxtapose(Sequence::stair(k - 1, k, -1));
            if (repeatBottom) {
                o.push_back(0);
            }
            if (repeatTop) {
                o.insert(o.begin(), k - 1);
            }
            o = o.juxtapose(Sequence::stair(1, k - 2, 1));
            break;
        case ArpType::SIGMAUP:
            for (int i = 0; i < k; ++i) {
                o = o.juxtapose(Sequence::stair(0, i + 1, 1));
            }
            break;
        case ArpType::SIGMADOWN:
            for (int i = 0; i < k; ++i) {
                o = o.juxtapose(Sequence::stair(k - 1, i + 1, -1));
            }
            break;
        }
        return o;
    }

    static Sequence parse(const std::string& s) {
        std::string str0 = s;
        str0.erase(std::remove_if(str0.begin(), str0.end(), ::isspace), str0.end());
        if (str0.front() == '[' && str0.back() == ']') {
            str0 = str0.substr(1, str0.length() - 2);
        }
        std::istringstream iss(str0);
        Sequence output;
        std::string token;
        while (std::getline(iss, token, ',')) {
            if (!token.empty()) {
                output.push_back(std::stoi(token));
            }
        }
        return output;
    }

    static Sequence stair(int o, int l, int a) {
        Sequence s;
        for (int i = 0; i < l; ++i) {
            s.push_back(o + i * a);
        }
        return s;
    }

    static Sequence tri(int o, int l, int a) {
        return stair(o, l, a).juxtapose(stair(o + l * a, l, -a));
    }

    Sequence getMininumRotation() const {
        Sequence s = *this;
        for (int i = 0; i < s.size(); ++i) {
            Sequence t = this->rotate(i);
            if (t < s) {
                s = t;
            }
        }
        return s;
    }

    Sequence convolveWith(const Sequence& impulse) const {
        Sequence o(this->size(), 0);
        for (int i = 0; i < this->size(); ++i) {
            for (int j = 0; j < impulse.size(); ++j) {
                int v = (*this)[i] * impulse[j];
                o[(i + j) % o.size()] += v;
            }
        }
        return o;
    }

    Sequence juxtapose(const Sequence& j) const {
        Sequence s = *this;
        s.insert(s.end(), j.begin(), j.end());
        return s;
    }

    Sequence wrapseq(int p_min, int p_amp) const {
        if (p_amp == 0) {
            throw std::runtime_error("Sequence.wrapseq: amp must be non-zero");
        }
        int d = 1;
        if (p_amp < 0) {
            d = -1;
            p_amp = std::abs(p_amp);
        }
        Sequence allowed = stair(p_min, p_amp, d);
        int asz = allowed.size();
        int min = this->getMin();
        int max = this->getMax();
        int minmap = p_min;
        while (minmap > min) {
            minmap -= asz;
        }
        int maxmap = p_min + p_amp;
        while (maxmap < max) {
            maxmap += asz;
        }
        std::map<int, int> map;
        int acc = 0;
        for (int i = minmap; i <= maxmap; ++i) {
            map[i] = allowed[Numbers::correctMod(acc++, allowed.size())];
        }
        return this->map(map);
    }

    Sequence bounceseq(int p_min, int p_amp) const {
        if (p_amp == 0) {
            throw std::runtime_error("Sequence.bounceseq: amp must be non-zero");
        }
        int d = 1;
        if (p_amp < 0) {
            d = -1;
            p_amp = std::abs(p_amp);
        }
        Sequence allowed = tri(p_min, p_amp, d);
        int asz = allowed.size();
        int min = this->getMin();
        int max = this->getMax();
        int minmap = p_min;
        while (minmap > min) {
            minmap -= asz;
        }
        int maxmap = p_min + p_amp;
        while (maxmap < max) {
            maxmap += asz;
        }
        std::map<int, int> map;
        int acc = 0;
        for (int i = minmap; i <= maxmap; ++i) {
            map[i] = allowed[acc++ % asz];
        }
        return this->map(map);
    }

    Sequence flip() const {
        return subFlip(*this, this->getMin(), this->getMax());
    }

    static Sequence subFlip(const Sequence& s, int l, int h) {
        Sequence o;
        for (int i = 0; i < s.size(); ++i) {
            o.push_back(h - (s[i] - l));
        }
        return o;
    }

    std::map<int, int> frequencyMap() const {
        std::map<int, int> o;
        for (int i : this->distinct()) {
            o[i] = this->count(i);
        }
        return o;
    }

    double entropy() const {
        double o = 0.0;
        std::map<int, int> freqs = this->frequencyMap();
        for (const auto& [key, value] : freqs) {
            double p = static_cast<double>(value) / static_cast<double>(this->size());
            o += p * std::log(p);
        }
        return (o == 0.0 ? 0.0 : -o);
    }

    int sumOfPairwiseDistances() const {
        int o = 0;
        for (int i = 0; i < this->size(); ++i) {
            for (int j = i; j < this->size(); ++j) {
                o += std::abs((*this)[i] - (*this)[j]);
            }
        }
        return o;
    }

    Sequence signs() const {
        Sequence s;
        for (int i = 0; i < this->size(); ++i) {
            if ((*this)[i] > 0) {
                s.push_back(1);
            }
            else if ((*this)[i] < 0) {
                s.push_back(-1);
            }
            else {
                s.push_back(0);
            }
        }
        return s;
    }

    Sequence multiply(int k) const {
        Sequence s;
        for (int i = 0; i < this->size(); ++i) {
            s.push_back((*this)[i] * k);
        }
        return s;
    }

    Sequence applyMin(int k) const {
        Sequence s;
        for (int i = 0; i < this->size(); ++i) {
            s.push_back(std::min((*this)[i], k));
        }
        return s;
    }

    Sequence applyMax(int k) const {
        Sequence s;
        for (int i = 0; i < this->size(); ++i) {
            s.push_back(std::max((*this)[i], k));
        }
        return s;
    }

    Sequence apply(const std::function<int(int)>& f) const {
        Sequence o;
        for (int i = 0; i < this->size(); ++i) {
            o.push_back(f((*this)[i]));
        }
        return o;
    }

    Matrix<bool> decompose() const {
        std::map<int, int> om = mapOrdinalsUnipolar();
        int m = this->distinct().size();
        int n = this->size();
        Matrix<bool> mat(m, n);
        for (int j = 0; j < n; ++j) {
            mat.set(om[(*this)[j]], j, true);
        }
        return mat;
    }

    Sequence powExp(int power) const {
        Sequence s;
        for (int i = 0; i < this->size(); ++i) {
            s.push_back(std::pow((*this)[i], power));
        }
        return s;
    }

    Sequence addToEach(const Sequence& s) const {
        int n = this->size() * s.size();
        Sequence o;
        for (int i = 0; i < n; ++i) {
            o.push_back((*this)[i % this->size()] + s[i / this->size()]);
        }
        return o;
    }

    Sequence addToEach(int k) const {
        Sequence output;
        for (int i : *this) {
            output.push_back(i + k);
        }
        return output;
    }

    Sequence powBase(int base) const {
        Sequence s;
        for (int i = 0; i < this->size(); ++i) {
            s.push_back(std::pow(base, (*this)[i]));
        }
        return s;
    }

    Sequence reverse() const {
        Sequence r;
        for (int i = this->size() - 1; i >= 0; --i) {
            r.push_back((*this)[i]);
        }
        return r;
    }

    Sequence rotateRight() const {
        return this->rotate(1);
    }

    Sequence rotateLeft() const {
        return this->rotate(-1);
    }

    Sequence rotate(int n) const {
        return Sequence::from(CollectionUtils::rotate(*this, n));
    }

    std::vector<int> getArray() const {
        return std::vector<int>(this->begin(), this->end());
    }

    int getMin() const {
        return *std::min_element(this->begin(), this->end());
    }

    int getMax() const {
        return *std::max_element(this->begin(), this->end());
    }

    double getMean() const {
        return std::accumulate(this->begin(), this->end(), 0.0) / this->size();
    }

    double getStdDev() const {
        double m = getMean();
        double s = 0;
        for (int i : *this) {
            double d = i - m;
            s += d * d;
        }
        return std::sqrt(s / this->size());
    }

    Sequence difference() const {
        return Sequence::from(CollectionUtils::difference(this->getArray()));
    }

    Sequence cyclicalDifference() const {
        return Sequence::from(CollectionUtils::cyclicalDifference(this->getArray()));
    }

    Sequence antidifference(int k) const {
        return Sequence::from(CollectionUtils::antidifference(this->getArray(), k));
    }

    Sequence cyclicalAntidifference(int k) const {
        return Sequence::from(CollectionUtils::cyclicalAntidifference(this->getArray(), k));
    }

    static Sequence from(const std::vector<int>& p_arr) {
        return Sequence(p_arr);
    }

    static Sequence from(const std::vector<int>& p_arr) {
        return Sequence(p_arr);
    }

    std::set<int> distinct() const {
        return std::set<int>(this->begin(), this->end());
    }

    int count(int n) const {
        return std::count(this->begin(), this->end(), n);
    }

    bool operator==(const Sequence& other) const {
        return static_cast<const std::vector<int>&>(*this) == static_cast<const std::vector<int>&>(other);
    }

    bool operator!=(const Sequence& other) const {
        return !(*this == other);
    }

    bool operator<(const Sequence& other) const {
        return std::lexicographical_compare(this->begin(), this->end(), other.begin(), other.end());
    }

    int compareTo(const Sequence& o) const {
        return std::lexicographical_compare(this->begin(), this->end(), o.begin(), o.end()) ? -1 : 1;
    }

    std::string toString(bool asJson = false) const {
        std::ostringstream oss;
        if (asJson) {
            oss << "[";
            for (size_t i = 0; i < this->size(); ++i) {
                oss << (*this)[i];
                if (i != this->size() - 1) {
                    oss << ", ";
                }
            }
            oss << "]";
        }
        else {
            for (size_t i = 0; i < this->size(); ++i) {
                oss << (*this)[i];
                if (i != this->size() - 1) {
                    oss << " ";
                }
            }
        }
        return oss.str();
    }

    int sum() const {
        return std::accumulate(this->begin(), this->end(), 0);
    }

    int getPeriod() const {
        int p = this->size();
        for (int i = 1; i < this->size(); ++i) {
            if (this->compareTo(this->rotate(i)) == 0) {
                p = i;
                break;
            }
        }
        return p;
    }

    Sequence circularHoldNonZero() const {
        Sequence o = *this;
        int last_non_zero = -1;
        for (int i = o.size(); i >= 0; --i) {
            if (o[i % o.size()] != 0) {
                last_non_zero = i % o.size();
                break;
            }
        }
        if (last_non_zero == -1) {
            std::fill(o.begin(), o.end(), 1);
            return o;
        }
        for (int i = 0; i < o.size(); ++i) {
            if (o[i] == 0) {
                int k = 1;
                while (o[(i - k + o.size()) % o.size()] == 0) {
                    k++;
                }
                int v = o[(i - k + o.size()) % o.size()];
                for (int j = 0; j < k; ++j) {
                    o[((i - j) + o.size()) % o.size()] = v;
                }
            }
        }
        return o;
    }

    int rangeSize() const {
        return this->getMax() - this->getMin() + 1;
    }

    std::map<int, int> mapOrdinalsUnipolar() const {
        std::map<int, int> o;
        std::set<int> d = this->distinct();
        int k = 1;
        for (int i : d) {
            o[i] = k++;
        }
        return o;
    }

    Sequence asOrdinalsUnipolar() const {
        return this->map(this->mapOrdinalsUnipolar());
    }

    std::map<int, int> mapOrdinalsBipolar() const {
        std::map<int, int> o;
        o[0] = 0;
        std::set<int> d = this->distinct();
        std::set<int> abs_d;
        for (int i : d) {
            abs_d.insert(std::abs(i));
        }
        abs_d.erase(0);
        int k = 1;
        for (int i : abs_d) {
            o[i] = k;
            o[-i] = -k;
            k++;
        }
        return o;
    }

    Sequence asOrdinalsBipolar() const {
        return this->map(this->mapOrdinalsBipolar());
    }

    std::map<int, Sequence> getIntervalVectors() const {
        return CollectionUtils::calcIntervalVector(*this);
    }

    Sequence map(const Sequence& s) const {
        std::map<int, int> t;
        for (int i = 0; i < s.size(); ++i) {
            t[i] = s[i];
            if (i > 0) {
                t[-i] = -s[i];
            }
        }
        return Sequence::map(*this, t);
    }

    Sequence map(const std::map<int, int>& i) const {
        return Sequence::map(*this, i);
    }

    Sequence permutate(const Sequence& s) const {
        Sequence o;
        for (int i = 0; i < s.size(); ++i) {
            o.push_back((*this)[s[i]]);
        }
        return o;
    }

    void addAtRandom(int v) {
        int p = RandomNumberGenerator::nextInt(this->size() + 1);
        this->insert(this->begin() + p, v);
    }

    Sequence copy() const {
        return *this;
    }

    Sequence rndRemove(int n) const {
        Sequence o = this->copy();
        if (n <= 0) {
            return o;
        }
        for (int i = 0; i < n; ++i) {
            int p = RandomNumberGenerator::nextInt(o.size());
            o.erase(o.begin() + p);
        }
        return o;
    }

    Sequence rndAdd(int n) const {
        Sequence o = this->copy();
        if (n <= 0) {
            return o;
        }
        int m = static_cast<int>(std::round(this->getMean()));
        int s = static_cast<int>(std::round(this->getStdDev()));
        for (int i = 0; i < n; ++i) {
            int v = static_cast<int>(std::round(RandomNumberGenerator::nextGaussian() * s + m));
            int p = RandomNumberGenerator::nextInt(o.size() + 1);
            o.insert(o.begin() + p, v);
        }
        return o;
    }

    static Sequence map(const Sequence& s, const std::map<int, int>& m) {
        Sequence output;
        for (int x : s) {
            output.push_back(m.at(x));
        }
        return output;
    }
    
    static int equivalenceShift(const Sequence& a, const Sequence& b) {
        if (a.size() != b.size()) {
            return -1;
        }
        for (int i = 0; i < a.size(); ++i) {
            if (b == a.rotate(i)) {
                return i;
            }
        }
        return -1;
    }

    static bool equivalentUnderRotation(const Sequence& a, const Sequence& b) {
        return equivalenceShift(a, b) != -1;
    }

    class ReverseComparator {
    public:
        int operator()(const Sequence& o1, const Sequence& o2) const {
            return -std::lexicographical_compare(o1.rbegin(), o1.rend(), o2.rbegin(), o2.rend());
        }
    };

    static Sequence genRnd(int vl, int amp, int sum, int maxamp, bool ex0, std::optional<int> start = std::nullopt) {
        if (vl <= 1 || amp < 1 || maxamp < 2 || (ex0 && maxamp < 2) || (std::abs(sum) >= maxamp) ||
            (amp == 1 && ex0 && (vl % 2) == 0 && (sum % 2) == 0)) {
            throw std::runtime_error("Sequence : genRnd check parameters.");
        }
        Sequence t;
        int currval = sum;
        t.push_back(currval);
        Sequence o;
        std::vector<int> possibles;
        while (true) {
            if (t.size() < vl) {
                while (true) {
                    possibles.clear();
                    for (int i = -amp; i <= amp; ++i) {
                        if ((std::abs(currval + i) < maxamp) && (i != 0 || !ex0)) {
                            possibles.push_back(i);
                        }
                    }
                    if (possibles.empty()) {
                        o.clear();
                        t.clear();
                        currval = sum;
                        t.push_back(currval);
                        break;
                    }
                    std::vector<double> weights(possibles.size());
                    for (size_t i = 0; i < weights.size(); ++i) {
                        weights[i] = 1.0 / (1 + t.count(currval + possibles[i])) * Numbers::factorial(std::abs(possibles[i]));
                    }
                    t.push_back(possibles.size() == 1 ? possibles[0] : CollectionUtils::chooseAtRandomWithWeights(possibles, weights) + currval);
                    currval = t.back();
                    break;
                }
            }
            else {
                o = t.reverse();
                if (!start || (start && o.front() == *start)) {
                    break;
                }
                else {
                    t.clear();
                    o.clear();
                    currval = sum;
                    t.push_back(currval);
                }
            }
        }
        return o;
    }


    int operator()(int t) const{
        return (*this)[t];
    }
};
