#pragma once

#include "Combination.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iterator>

class Composition : public Combination {
public:
    Composition(const std::vector<bool>& comp);

    Composition(int n);

    Composition();

    Composition(const std::bitset<128>& x, int n);

    Composition(const Combination& c);

    int getTotal() const;

    Sequence asSequence() const;

    Combination asCombination() const;

    template <typename T>
    std::vector<std::vector<T>> segmentList(const std::vector<T>& s) const;

    std::vector<std::string> segmentString(const std::string& str) const;

    std::string toString() const;

    static std::vector<Composition> refinements(const Composition& co);
    static Composition getCompositionFromCombination(const Combination& c);

private:
    int m_n;
};
