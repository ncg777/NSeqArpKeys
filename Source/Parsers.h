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

class Parsers {
public:
    static std::function<std::string(const std::string&)> stringParser;
    static std::function<int(const std::string&)> intParser;
    static std::function<int(const std::string&)> integerParser;
    static std::function<double(const std::string&)> doubleParser;
    static std::function<std::vector<double>(const std::string&)> doubleArrayParser;
    static std::function<std::vector<int>(const std::string&)> integerArrayParser;
    static std::function<Combination(const std::string&)> combinationParser;
    static std::function<Composition(const std::string&)> compositionParser;
    static std::function<std::vector<int>(const std::string&)> intArrayParser;
    static std::function<Sequence(const std::string&)> sequenceParser;
    static std::function<HomoPair<int>(const std::string&)> intPairParser;


    template <typename T>
    static std::function<T(const std::string&)> quoteRemoverDecorator(std::function<T(const std::string&)> parser) {
        return [parser](const std::string& s) {
            std::string u = s.substr(1, s.length() - 2);
            return parser(u);
            };
    }


    template <typename X>
    static std::function<X(const std::string&)> nullDecorator(std::function<X(const std::string&)> parser) {
        return [parser](const std::string& s) {
            return s == "null" ? X() : parser(s);
            };
    }

    template <typename T, typename U>
    static std::function<HeteroPair<T, U>(const std::string&)> heteroPairDecorator(std::function<T(const std::string&)> parser1, std::function<U(const std::string&)> parser2) {
        return [parser1, parser2](const std::string& s) {
            return HeteroPair<T, U>::parseJSONObject(s, parser1, parser2);
            };
    }

    template <typename T>
    static std::function<HomoPair<T>(const std::string&)> homoPairDecorator(std::function<T(const std::string&)> parser) {
        return [parser](const std::string& s) {
            return HomoPair<T>::fromHeteroPair(HeteroPair<T, T>::parseJSONObject(s, parser, parser));
            };
    }
};

std::function<std::string(const std::string&)> Parsers::stringParser = [](const std::string& s) { return s; };
std::function<int(const std::string&)> Parsers::intParser = [](const std::string& value) { return std::stoi(value); };
std::function<int(const std::string&)> Parsers::integerParser = [](const std::string& s) { return std::stoi(s); };
std::function<double(const std::string&)> Parsers::doubleParser = [](const std::string& s) { return std::stod(s); };
std::function<std::vector<int>(const std::string&)> Parsers::intArrayParser = [](const std::string& s) {
    std::istringstream iss(s);
    std::vector<int> result((std::istream_iterator<int>(iss)), std::istream_iterator<int>());
    return result;
    };
std::function<Sequence(const std::string&)> Parsers::sequenceParser = [](const std::string& s) { return Sequence::parse(s); };
std::function<HomoPair<int>(const std::string&)> Parsers::intPairParser = [](const std::string& s) {
    Sequence ss = Sequence::parse(s);
    return HomoPair<int>::makeHomoPair(ss[0], ss[1]);
    };

