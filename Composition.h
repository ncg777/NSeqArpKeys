#pragma once

#include "Combination.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iterator>

class Composition : public Combination {
public:
    Composition(const std::vector<bool>& comp) : Combination(comp.size()) {
        for (int i = 0; i < m_n; ++i) {
            if (comp[i]) {
                this->set(i, comp[i]);
            }
        }
    }

    Composition(int n) : Combination(n - 1) {}

    Composition() : Combination(0) {}

    Composition(const std::bitset<128>& x, int n) : Combination(x, n - 1) {}

    Composition(const Combination& c) : Combination(c) {}

    int getTotal() const {
        return m_n + 1;
    }

    Sequence asSequence() const {
        Sequence o;
        int n = 1;
        for (int i = 0; i < m_n; ++i) {
            if (this->test(i)) {
                o.push_back(n);
                n = 1;
            }
            else {
                n++;
            }
        }
        o.push_back(n);
        return o;
    }

    Combination asCombination() const {
        Combination o(m_n + 1);
        o.set(0);
        for (int i = 1; i < m_n + 1; ++i) {
            o.set(i, this->test(i - 1));
        }
        return o;
    }

    template <typename T>
    std::vector<std::vector<T>> segmentList(const std::vector<T>& s) const {
        if (this->getTotal() != s.size()) throw std::invalid_argument("List size does not match composition size");
        std::vector<std::vector<T>> o;
        Sequence cs = this->asSequence();
        int start = 0;
        for (int i = 0; i < cs.size(); ++i) {
            o.push_back(std::vector<T>(s.begin() + start, s.begin() + start + cs[i]));
            start += cs[i];
        }
        return o;
    }

    std::vector<std::string> segmentString(const std::string& str) const {
        if (this->getTotal() != str.length()) throw std::invalid_argument("String length does not match composition size");
        std::vector<std::string> o;
        Sequence s = this->asSequence();
        int start = 0;
        for (int i = 0; i < s.size(); ++i) {
            o.push_back(str.substr(start, s[i]));
            start += s[i];
        }
        return o;
    }

    std::string toString() const {
        return asSequence().toString();
    }

    static std::vector<Composition> refinements(const Composition& co) {
        std::vector<Combination> c = Combination::refinements(co);
        if (c.empty()) {
            return {};
        }
        std::vector<Composition> o(c.size());
        for (int i = 0; i < c.size(); ++i) {
            o[i] = Composition(c[i]);
        }
        return o;
    }
    static Composition getCompositionFromCombination(const Combination& c) {
        int nsb = c.find_first();
        if (nsb == -1) {
            return Composition(c.getN());
        }
        else {
            Combination t = c.rotate(nsb);
            std::vector<bool> l;
            for (int i = 1; i < c.getN(); ++i) {
                l.push_back(t.test(i));
            }
            return Composition(l);
        }
    }

private:
    int m_n;
};

