#include "Printers.h"

std::function<std::string(const std::string&)> Printers::stringPrinter = [](const std::string& s) { return s; };
std::function<std::string(int)> Printers::integerPrinter = [](int i) { return std::to_string(i); };
std::function<std::string(int)> Printers::intPrinter = [](int value) { return std::to_string(value); };
std::function<std::string(double)> Printers::doublePrinter = [](double d) { return std::to_string(d); };
std::function<std::string(const std::vector<double>&)> Printers::doubleArrayPrinter = [](const std::vector<double>&) { return std::string(); };
std::function<std::string(const std::vector<int>&)> Printers::integerArrayPrinter = [](const std::vector<int>&) { return std::string(); };
std::function<std::string(const Sequence&)> Printers::sequencePrinter = [](const Sequence& s) { return s.toString(); };
std::function<std::string(const HomoPair<int>&)> Printers::intPairPrinter = [](const HomoPair<int>& p) {
    Sequence ss;
    ss.push_back(p.getFirst());
    ss.push_back(p.getSecond());
    return ss.toString();
    };
