#pragma once

#include <string>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include "Combination.h"
#include "ImmutableCombination.h"
#include "Necklace.h"
#include "Sequence.h"
#include "ForteCSV.h"
#include "FiniteRelation.h"
#include "Parsers.h"

class Pcs12 : public ImmutableCombination {
public:
    int getN() const {
        return combination.getN();
    }

    int getK() const {
        return combination.getK();
    }

    int calcSpan() const {
        return combination.calcSpan();
    }

    Sequence getIntervalVector() const {
        return combination.getIntervalVector();
    }

    double calcNormalizedDistanceWith(const Pcs12& other) const {
        return combination.calcNormalizedDistanceWith(other.combination);
    }

    Pcs12 symmetricDifference(const Pcs12& y) const {
        return Pcs12::identify(combination.symmetricDifference(y.combination));
    }

    Pcs12 rotate(int t) const {
        return Pcs12::identify(combination.rotate(t));
    }

    Pcs12 intersect(const Pcs12& c) const {
        return Pcs12::identify(combination.intersect(c.combination));
    }

    Pcs12 minus(const Pcs12& c) const {
        return Pcs12::identify(combination.minus(c.combination));
    }

    std::vector<Pcs12> partition(const std::vector<int>& partition) const {
        std::vector<Combination> result = combination.partition(partition);
        return toPcs12Combinations(result);
    }

    std::vector<Pcs12> partition(const Sequence& p0) const {
        std::vector<Combination> result = combination.partition(p0);
        return toPcs12Combinations(result);
    }

    static std::vector<Pcs12> toPcs12Combinations(const std::vector<Combination>& combinations) {
        std::vector<Pcs12> pcs12Combinations(combinations.size());
        for (size_t i = 0; i < combinations.size(); ++i) {
            pcs12Combinations[i] = Pcs12::identify(combinations[i]);
        }
        return pcs12Combinations;
    }

    Sequence asSequence() const {
        return combination.asSequence();
    }

    std::set<int> asSet() const {
        return combination.asSet();
    }

    Sequence asBinarySequence() const {
        return combination.asBinarySequence();
    }

    Pcs12 merge(const Pcs12& other) const {
        return Pcs12::identify(Combination::merge(combination, other.combination));
    }

    static Pcs12 fromBinarySequence(const Sequence& s) {
        Combination c = Combination::fromBinarySequence(s);
        return Pcs12::identify(c);
    }

    bool get(int bitIndex) const {
        return combination.test(bitIndex);
    }

    bool isEmpty() const {
        return combination.none();
    }

    std::string toString() const {
        return combination.toString();
    }

    Combination getCombinationCopy() const {
        return Combination(combination);
    }

    bool operator==(const Pcs12& other) const {
        return combination == other.combination;
    }

    bool operator!=(const Pcs12& other) const {
        return !(*this == other);
    }

    int compareTo(const Pcs12& o) const {
        return combination.compareTo(o.combination);
    }

    static Pcs12 identify(const Combination& input) {
        return identify(ImmutableCombination::fromCombination(input));
    }

    static Pcs12 identify(const ImmutableCombination& input) {
        if (input.getN() != 12) {
            throw std::invalid_argument("Pcs12::IdentifyChord the combination is not bounded by 12");
        }

        if (input.isEmpty()) {
            return empty();
        }

        return ChordDict.at(input.toString());
    }

    static Pcs12 identify(const Sequence& input) {
        return Pcs12::identify(std::set<int>(input.begin(), input.end()));
    }

    static Pcs12 identify(const std::set<int>& input) {
        if (input.empty()) {
            return Pcs12::empty();
        }

        bool ex = false;
        for (const auto& tmp : input) {
            if (tmp > 11 || tmp < 0) {
                ex = true;
                break;
            }
        }
        if (input.size() > 12) {
            ex = true;
        }
        if (ex) {
            throw std::invalid_argument("Pcs12::IdentifyChord Provided set is not a valid chord.");
        }

        return identify(ImmutableCombination(12, input));
    }

    static Pcs12 empty() {
        return Pcs12(std::set<int>(), 1, 0);
    }

    static std::set<Pcs12> generate() {
        std::vector<int> o(12, 0);
        std::set<Pcs12> output;
        std::set<Necklace> nck = Necklace::generate(12, 2);
        std::vector<Necklace> arr(nck.begin(), nck.end());

        std::sort(arr.begin(), arr.end(), Sequence::ReverseComparator());

        for (const auto& n : arr) {
            int period = n.getPeriod();
            int sz_tmp = std::count(n.begin(), n.end(), 1);

            for (int j = 0; j < period; ++j) {
                std::set<int> c;
                for (int k = 0; k < 12; ++k) {
                    if (n[k] == 1) {
                        c.insert(((12 - (k + 1)) + j) % 12);
                    }
                }
                if (!c.empty()) {
                    output.insert(Pcs12(c, o[sz_tmp - 1] + 1, j));
                }
            }
            if (sz_tmp != 0) {
                o[sz_tmp - 1]++;
            }
        }

        output.insert(Pcs12::empty());

        return output;
    }

    static void GenerateMaps() {
        std::set<Pcs12> t = generate();

        std::map<std::string, Pcs12> output;
        for (const auto& tmp : t) {
            output[tmp.toString()] = tmp;
        }
        ChordDict = output;
        fillForteNumbersDict();
    }

    static void fillForteNumbersDict() {
        ForteNumbersDict.clear();
        ForteNumbersRotationDict.clear();
        ForteNumbersToPCS12Dict.clear();
        FiniteRelation<std::string, Sequence> r = FiniteRelation<std::string, Sequence>::readFromCSV(
            Parsers::stringParser, Parsers::sequenceParser, std::istringstream(ForteCSV::FORTE_NUMBERS));

        for (const auto& p : r) {
            Pcs12 ch = Pcs12::identify(p.getSecond());

            for (int i = 0; i < 12; ++i) {
                Pcs12 t = ch.transpose(-i);
                if (ForteNumbersDict.find(t) == ForteNumbersDict.end()) {
                    ForteNumbersDict[t] = p.getFirst();
                    ForteNumbersRotationDict[t] = i;
                    ForteNumbersToPCS12Dict[p.getFirst() + "." + std::to_string(i)] = t;
                }
            }
        }

        FiniteRelation<std::string, std::string> r2 = FiniteRelation<std::string, std::string>::readFromCSV(
            Parsers::stringParser, Parsers::stringParser, std::istringstream(ForteCSV::COMMON_NAMES));

        ForteNumbersCommonNames.clear();
        for (const auto& p : r2) {
            ForteNumbersCommonNames[p.getFirst()] = p.getSecond();
        }
    }

    static std::map<std::string, Pcs12> getChordDict() {
        return ChordDict;
    }

    static std::map<std::string, Pcs12> getForteChordDict() {
        return ForteNumbersToPCS12Dict;
    }

    static std::set<Pcs12> getChords() {
        std::set<Pcs12> output;
        for (const auto& pair : ChordDict) {
            output.insert(pair.second);
        }
        return output;
    }

    static Pcs12 parseForte(const std::string& input) {
        return ForteNumbersToPCS12Dict.at(input);
    }

    std::string getForteAB() const {
        std::string f = getForteNumber();
        return f.find("A") != std::string::npos ? "A" : f.find("B") != std::string::npos ? "B" : "";
    }

    int rotatedCompareTo(const Pcs12& other, int rotate) const {
        return this->asBinarySequence().rotate(rotate).compareTo(other.asBinarySequence().rotate(rotate));
    }

    int calcDistanceWith(const Pcs12& other) const {
        int maxn = std::max(this->getN(), other.getN());
        int acc = 0;
        for (int i = 0; i < maxn; ++i) {
            if (this->get(i) != other.get(i)) {
                acc++;
            }
        }
        return acc;
    }

    Pcs12 transpose(int t) const {
        return identify(this->rotate(t));
    }

    Pcs12 S12Permutate(const Sequence& s) const {
        if (s.distinct().size() != 12 || s.getMin() != 0 || s.getMax() != 11) {
            throw std::runtime_error("Invalid permutation");
        }
        return Pcs12::identify(ImmutableCombination::fromBinarySequence(this->asBinarySequence().permutate(s)));
    }

    double getMean() const {
        double s = 0.0;
        double k = getK();

        for (int i = 0; i < 12; ++i) {
            if (get(i)) {
                s += i;
            }
        }
        return s / k;
    }

    std::string combinationString() const {
        return combination.toString();
    }

    int getOrder() const {
        return m_Order;
    }

    int getTranspose() const {
        return m_Transpose;
    }

    Pcs12 combineWith(const Pcs12& x) const {
        return Pcs12::identify(this->merge(x));
    }

    std::string getCommonName() const {
        return ForteNumbersCommonNames.at(getForteNumber());
    }

    std::string getForteNumber() const {
        return ForteNumbersDict.at(*this);
    }

    int getForteNumberOrder() const {
        std::string str = getForteNumber();
        str.erase(std::remove(str.begin(), str.end(), 'z'), str.end());
        str.erase(std::remove(str.begin(), str.end(), 'A'), str.end());
        str.erase(std::remove(str.begin(), str.end(), 'B'), str.end());
        str = str.substr(str.find("-") + 1);
        return std::stoi(str);
    }

    int getForteNumberRotation() const {
        return ForteNumbersRotationDict.at(*this);
    }

    std::string toForteNumberString() const {
        return getForteNumber() + "." + std::to_string(getForteNumberRotation());
    }

    std::vector<double> getSymmetries() const {
        if (symmetries.has_value()) {
            return symmetries.value();
        }

        std::vector<double> o;

        for (int i = 0; i < 24; ++i) {
            int axis = i / 2;
            bool found = true;
            if (i % 2 == 0) {
                for (int j = 0; j < 7; ++j) {
                    if (this->get((axis + j) % 12) != this->get((12 + axis - j) % 12)) {
                        found = false;
                        break;
                    }
                }
            }
            else {
                for (int j = 0; j < 6; ++j) {
                    if (this->get((axis + j + 1) % 12) != this->get((12 + axis - j) % 12)) {
                        found = false;
                        break;
                    }
                }
            }
            if (found) {
                o.push_back(static_cast<double>(i) / 2.0);
            }
        }

        symmetries = o;
        return o;
    }

    double calcCenterTuning(int center) const {
        double o = 1.0;
        Sequence s = this->asSequence();
        std::map<int, double> m;
        for (int i = -5; i < 7; ++i) {
            m[(center + i + 12) % 12] = std::pow(2.0, static_cast<double>(i) / 12.0);
        }
        for (const auto& t : s) {
            o *= m[t];
        }
        return std::pow(o, 1.0 / static_cast<double>(s.size()));
    }

    static std::vector<std::string> CommonScales() {
        std::vector<std::string> o;
        for (const auto& ch : getChords()) {
            if (ch.getK() == 7) {
                if (ch.getOrder() == 26 || // "07-26.04", Major Locrian
                    ch.getOrder() == 28 || // "07-28.11", Persian
                    ch.getOrder() == 29 || // "07-29.06", Hungarian
                    ch.getOrder() == 38 || // "07-38.11", Harmonic minor
                    ch.getOrder() == 39 || // "07-39.11", Melodic minor
                    ch.getOrder() == 42 || // "07-42.11", Harmonic major
                    ch.getOrder() == 43)   // "07-43.11", Major
                {
                    o.push_back(ch.toForteNumberString());
                }
            }
            if (ch.getK() == 8) {
                if (ch.getOrder() == 35) // "08-35.00", Octatonic
                {
                    o.push_back(ch.toForteNumberString());
                }
            }
            if (ch.getK() == 12) {
                o.push_back(ch.toForteNumberString());
            }
        }
        std::sort(o.begin(), o.end());
        return o;
    }
    static bool ForteStringComparator(const std::string& o1, const std::string& o2) {
        Pcs12 p1 = Pcs12::parseForte(o1);
        Pcs12 p2 = Pcs12::parseForte(o2);

        if (p1.getK() != p2.getK()) {
            return p1.getK() < p2.getK();
        }
        if (p1.getForteNumberOrder() != p2.getForteNumberOrder()) {
            return p1.getForteNumberOrder() < p2.getForteNumberOrder();
        }
        if (p1.getForteAB() != p2.getForteAB()) {
            return p1.getForteAB() < p2.getForteAB();
        }
        return p1.getForteNumberRotation() < p2.getForteNumberRotation();
    }

    static std::vector<std::string> getSortedForteNumbers() {
        std::vector<std::string> forteNumbers;
        for (const auto& pair : ForteNumbersToPCS12Dict) {
            forteNumbers.push_back(pair.first);
        }
        std::sort(forteNumbers.begin(), forteNumbers.end(), ForteStringComparator);
        std::reverse(forteNumbers.begin(), forteNumbers.end());
        return forteNumbers;
    }
    Pcs12() : m_Order(1), m_Transpose(0){
    
    }
private:
    int m_Order;
    int m_Transpose;
    mutable std::optional<std::vector<double>> symmetries;
    Pcs12(const std::set<int>& p_s, int p_Order, int p_Transpose)
        : ImmutableCombination(12, p_s), m_Order(p_Order), m_Transpose(p_Transpose) {
    }
    static std::map<std::string, Pcs12> ChordDict;
    static std::map<Pcs12, std::string> ForteNumbersDict;
    static std::map<Pcs12, int> ForteNumbersRotationDict;
    static std::map<std::string, Pcs12> ForteNumbersToPCS12Dict;
    static std::map<std::string, std::string> ForteNumbersCommonNames;
};

