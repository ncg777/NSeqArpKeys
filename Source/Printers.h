#pragma once

#include <string>
#include <functional>
#include <vector>
#include <sstream>
#include <iterator>
#include <algorithm>
#include "HeteroPair.h"
#include "HomoPair.h"
#include "Combination.h"
#include "Composition.h"
#include "Sequence.h"

class Printers {
public:
    static std::function<std::string(const std::string&)> stringPrinter;
    static std::function<std::string(int)> integerPrinter;
    static std::function<std::string(int)> intPrinter;
    static std::function<std::string(double)> doublePrinter;
    static std::function<std::string(const std::vector<double>&)> doubleArrayPrinter;
    static std::function<std::string(const std::vector<int>&)> integerArrayPrinter;
    static std::function<std::string(const Sequence&)> sequencePrinter;
    static std::function<std::string(const Combination&)> combinationPrinter;
    static std::function<std::string(const Composition&)> compositionPrinter;
    static std::function<std::string(const HomoPair<int>&)> intPairPrinter;

    template <typename T>
    static std::function<std::string(const std::vector<T>&)> listPrinter(std::function<std::string(T)> printer) {
        return listPrinter(printer, " ");
    }

    template <typename T>
    static std::function<std::string(const std::vector<T>&)> listPrinter(std::function<std::string(T)> printer, const std::string& separator) {
        return [printer, separator](const std::vector<T>& l) {
            std::ostringstream oss;
            std::copy(l.begin(), l.end(), std::ostream_iterator<std::string>(oss, separator.c_str()));
            return oss.str();
            };
    }

    template <typename T, typename U>
    static std::function<std::string(const HeteroPair<T, U>&)> heteroPairDecorator(std::function<std::string(T)> printer1, std::function<std::string(U)> printer2) {
        return [printer1, printer2](const HeteroPair<T, U>& p) {
            return p.toString(printer1, printer2);
            };
    }

    template <typename T>
    static std::function<std::string(const HomoPair<T>&)> homoPairDecorator(std::function<std::string(T)> printer) {
        return [printer](const HomoPair<T>& p) {
            return p.toString(printer, printer);
            };
    }
};

std::function<std::string(const std::string&)> Printers::stringPrinter = [](const std::string& s) { return s; };
std::function<std::string(int)> Printers::integerPrinter = [](int i) { return std::to_string(i); };
std::function<std::string(int)> Printers::intPrinter = [](int value) { return std::to_string(value); };
std::function<std::string(double)> Printers::doublePrinter = [](double d) { return std::to_string(d); };
std::function<std::string(const std::vector<double>&)> Printers::doubleArrayPrinter = Printers::arrayDecorator<double>(Printers::doublePrinter);
std::function<std::string(const std::vector<int>&)> Printers::integerArrayPrinter = Printers::arrayDecorator<int>(Printers::integerPrinter);
std::function<std::string(const Sequence&)> Printers::sequencePrinter = [](const Sequence& s) { return s.toString(); };
std::function<std::string(const HomoPair<int>&)> Printers::intPairPrinter = [](const HomoPair<int>& p) {
    Sequence ss;
    ss.push_back(p.getFirst());
    ss.push_back(p.getSecond());
    return ss.toString();
    };


