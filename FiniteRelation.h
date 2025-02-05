#pragma once

#include <set>
#include <map>
#include <vector>
#include <functional>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include "HeteroPair.h"
#include "JaggedList.h"
#include "Parsers.h"
#include "Printers.h"
#include "CollectionUtils.h"

template <typename X, typename Y>
class FiniteRelation : public Relation<X, Y>, public std::enable_shared_from_this<FiniteRelation<X, Y>> {
public:
    FiniteRelation() = default;

    FiniteRelation(const std::map<X, Y>& map) {
        for (const auto& e : map) {
            add(e.first, e.second);
        }
    }

    FiniteRelation(const FiniteRelation<X, Y>& rel) {
        pairs = rel.pairs;
        pairsReversed = rel.pairsReversed;
    }

    FiniteRelation(const std::vector<X>& domain, const std::vector<Y>& codomain, std::function<bool(X, Y)> rel) {
        for (const auto& p : HeterogeneousPairEnumeration<X, Y>(domain, codomain)) {
            if (rel(p.first, p.second)) {
                add(p.first, p.second);
            }
        }
    }

    FiniteRelation(const std::vector<X>& domain, const std::vector<Y>& codomain, const Relation<X, Y>& rel) {
        for (const auto& p : HeterogeneousPairEnumeration<X, Y>(domain, codomain)) {
            if (rel.apply(p.first, p.second)) {
                add(p.first, p.second);
            }
        }
    }

    FiniteRelation(const std::vector<X>& domain, std::function<Y(X)> f) {
        for (const auto& x : domain) {
            add(x, f(x));
        }
    }

    static FiniteRelation<X, Y> evaluateParametric(std::function<HeteroPair<X, Y>(double)> f, double delta) {
        return evaluateParametric(f, delta, 0.0, 1.0);
    }

    static FiniteRelation<X, Y> evaluateParametric(std::function<HeteroPair<X, Y>(double)> f, double delta, double startInclusive, double endExclusive) {
        double c = startInclusive;
        FiniteRelation<X, Y> o;
        while (c < endExclusive) {
            auto p = f(c);
            c += delta;
            o.add(p.first, p.second);
        }
        return o;
    }

    bool operator==(const FiniteRelation<X, Y>& other) const {
        return size() == other.size() && std::all_of(other.pairs.begin(), other.pairs.end(), [this](const HeteroPair<X, Y>& p) { return contains(p); });
    }

    bool operator!=(const FiniteRelation<X, Y>& other) const {
        return !(*this == other);
    }

    static FiniteRelation<X, Y> universal(const std::vector<X>& domain, const std::vector<Y>& codomain) {
        return FiniteRelation<X, Y>(domain, codomain, [](X a, Y b) { return true; });
    }

    FiniteRelation<X, Y> complement(const std::vector<X>& domain, const std::vector<Y>& codomain) const {
        auto u = universal(domain, codomain);
        if (!std::includes(u.pairs.begin(), u.pairs.end(), pairs.begin(), pairs.end())) {
            throw std::runtime_error("the relation exceeds the specified universe.");
        }
        return u.minus(*this);
    }

    FiniteRelation<X, Y> complement() const {
        return complement(domain(), codomain());
    }

    int size() const {
        return pairs.size();
    }

    bool isEmpty() const {
        return pairs.empty();
    }

    bool contains(const HeteroPair<X, Y>& p) const {
        return pairs.find(p) != pairs.end();
    }

    bool add(const X& a, const Y& b) {
        auto p = HeteroPair<X, Y>(a, b);
        bool o = pairs.insert(p).second;
        if (o) {
            pairsReversed.insert(p.converse());
        }
        domain.insert(a);
        codomain.insert(b);
        return o;
    }

    bool remove(const X& a, const Y& b) {
        auto p = HeteroPair<X, Y>(a, b);
        bool o = pairs.erase(p) > 0;
        if (o) {
            pairsReversed.erase(p.converse());
        }
        if (pairs.find(HeteroPair<X, Y>(a, Y())) == pairs.end()) {
            domain.erase(a);
        }
        if (pairsReversed.find(HeteroPair<Y, X>(b, X())) == pairsReversed.end()) {
            codomain.erase(b);
        }
        return o;
    }

    void spawnRight(std::function<std::vector<Y>(X)> multiplier, const std::set<X>& domain) {
        for (const auto& x : domain) {
            for (const auto& s : multiplier(x)) {
                add(x, s);
            }
        }
    }

    void spawnRight(std::function<std::vector<Y>(X)> multiplier) {
        spawnRight(multiplier, domain());
    }

    void spawnLeft(std::function<std::vector<X>(Y)> multiplier, const std::set<Y>& codomain) {
        for (const auto& y : codomain) {
            for (const auto& s : multiplier(y)) {
                add(s, y);
            }
        }
    }

    void spawnLeft(std::function<std::vector<X>(Y)> multiplier) {
        spawnLeft(multiplier, codomain());
    }

    bool containsAll(const FiniteRelation<X, Y>& r) const {
        return std::includes(pairs.begin(), pairs.end(), r.pairs.begin(), r.pairs.end());
    }

    FiniteRelation<X, Y> intersect(const FiniteRelation<X, Y>& S) const {
        FiniteRelation<X, Y> o;
        std::set_intersection(pairs.begin(), pairs.end(), S.pairs.begin(), S.pairs.end(), std::inserter(o.pairs, o.pairs.begin()));
        return o;
    }

    FiniteRelation<X, Y> unionWith(const FiniteRelation<X, Y>& S) const {
        FiniteRelation<X, Y> o;
        std::set_union(pairs.begin(), pairs.end(), S.pairs.begin(), S.pairs.end(), std::inserter(o.pairs, o.pairs.begin()));
        return o;
    }

    FiniteRelation<X, Y> minus(const FiniteRelation<X, Y>& e) const {
        FiniteRelation<X, Y> o(*this);
        for (const auto& p : e) {
            o.remove(p.first, p.second);
        }
        return o;
    }

    template <typename V>
    FiniteRelation<X, V> compose(const FiniteRelation<Y, V>& S) const {
        FiniteRelation<X, V> o;
        for (const auto& xv : CollectionUtils::cartesianProduct(domain(), S.codomain())) {
            std::set<Y> common = rightRelata(xv.first);
            std::set<Y> commonS = S.leftRelata(xv.second);
            std::set<Y> intersection;
            std::set_intersection(common.begin(), common.end(), commonS.begin(), commonS.end(), std::inserter(intersection, intersection.begin()));
            if (!intersection.empty()) {
                o.add(xv.first, xv.second);
            }
        }
        return o;
    }

    FiniteRelation<Y, X> converse() const {
        FiniteRelation<Y, X> o;
        for (const auto& p : pairs) {
            o.add(p.second, p.first);
        }
        return o;
    }

    template <typename W>
    FiniteRelation<Y, W> rightResidual(const FiniteRelation<X, W>& S) const {
        FiniteRelation<Y, W> o;
        for (const auto& vw : CollectionUtils::cartesianProduct(codomain(), S.codomain())) {
            std::set<X> xSw = S.leftRelata(vw.second);
            std::set<X> xRv = leftRelata(vw.first);
            if (std::includes(xSw.begin(), xSw.end(), xRv.begin(), xRv.end())) {
                o.add(vw.first, vw.second);
            }
        }
        return o;
    }

    template <typename V>
    FiniteRelation<X, V> leftResidual(const FiniteRelation<V, Y>& R) const {
        FiniteRelation<X, V> o;
        for (const auto& uv : CollectionUtils::cartesianProduct(domain(), R.domain())) {
            std::set<Y> uSy = rightRelata(uv.first);
            std::set<Y> vRy = R.rightRelata(uv.second);
            if (std::includes(uSy.begin(), uSy.end(), vRy.begin(), vRy.end())) {
                o.add(uv.first, uv.second);
            }
        }
        return o;
    }

    std::set<X> domain() const {
        return domain;
    }

    bool domainCovers(const std::set<X>& s) const {
        return std::includes(domain.begin(), domain.end(), s.begin(), s.end());
    }

    std::set<Y> codomain() const {
        return codomain;
    }

    bool codomainCovers(const std::set<Y>& s) const {
        return std::includes(codomain.begin(), codomain.end(), s.begin(), s.end());
    }

    std::function<bool(X, Y)> related() const {
        return [this](X t, Y u) { return pairs.find(HeteroPair<X, Y>(t, u)) != pairs.end(); };
    }

    std::function<bool(Y)> rightRelated(const X& e) const {
        return [this, e](Y u) { return related()(e, u); };
    }

    std::set<Y> rightRelata(const X& e) const {
        std::set<Y> o;
        auto it = pairs.lower_bound(HeteroPair<X, Y>(e, Y()));
        while (it != pairs.end() && it->first == e) {
            o.insert(it->second);
            ++it;
        }
        return o;
    }

    std::function<bool(X)> leftRelated(const Y& e) const {
        return [this, e](X t) { return related()(t, e); };
    }

    std::set<X> leftRelata(const Y& e) const {
        std::set<X> o;
        auto it = pairsReversed.lower_bound(HeteroPair<Y, X>(e, X()));
        while (it != pairsReversed.end() && it->first == e) {
            o.insert(it->second);
            ++it;
        }
        return o;
    }

    bool isLeftUnique(const std::set<Y>& codomain) const {
        return std::none_of(codomain.begin(), codomain.end(), [this](const Y& y) { return leftRelata(y).size() > 1; });
    }

    bool isLeftUnique() const {
        return isLeftUnique(codomain());
    }

    bool isInjective() const {
        return isLeftUnique();
    }

    bool isLeftTotal(const std::set<X>& domain) const {
        return std::all_of(domain.begin(), domain.end(), [this](const X& x) { return !rightRelata(x).empty(); });
    }

    bool isRightUnique(const std::set<X>& domain) const {
        return std::none_of(domain.begin(), domain.end(), [this](const X& x) { return rightRelata(x).size() > 1; });
    }

    bool isRightUnique() const {
        return isRightUnique(domain());
    }

    bool isFunctional() const {
        return isRightUnique(domain());
    }

    bool isSurjective(const std::set<Y>& codomain) const {
        return std::all_of(codomain.begin(), codomain.end(), [this](const Y& y) { return !leftRelata(y).empty(); });
    }

    bool isFunctional(const std::set<X>& domain) const {
        return isRightUnique(domain) && isLeftTotal(domain);
    }

    bool isManyToMany() const {
        return !isLeftUnique() && !isRightUnique();
    }

    bool isManyToOne() const {
        return !isLeftUnique() && isRightUnique();
    }

    bool isOneToMany() const {
        return isLeftUnique() && !isRightUnique();
    }

    bool isOneToOne() const {
        return isLeftUnique() && isRightUnique();
    }

    static void writeToCSV(const FiniteRelation<X, Y>& finiteBinaryRelation, std::function<std::string(X)> xToString, std::function<std::string(Y)> yToString, const std::string& path, bool useBase64) {
        std::ofstream p(path);
        io::CSVWriter<2> w(p);
        auto xp = [xToString](X x) { return x == X() ? "null" : xToString(x); };
        auto yp = [yToString](Y y) { return y == Y() ? "null" : yToString(y); };

        if (useBase64) {
            xp = Printers::base64Decorator(xp);
            yp = Printers::base64Decorator(yp);
        }

        for (const auto& t : finiteBinaryRelation.pairs) {
            w << xp(t.first) << yp(t.second);
        }
    }

    void writeToCSV(std::function<std::string(X)> xFunction, std::function<std::string(Y)> yToString, const std::string& path, bool useBase64) const {
        writeToCSV(*this, xFunction, yToString, path, useBase64);
    }

    void writeToCSV(std::function<std::string(X)> xToString, std::function<std::string(Y)> yToString, const std::string& path) const {
        writeToCSV(*this, xToString, yToString, path, false);
    }

    static FiniteRelation<X, Y> readFromCSV(std::function<X(std::string)> xParser, std::function<Y(std::string)> yParser, std::istream& is, bool useBase64) {
        io::CSVReader<2> r(is);
        FiniteRelation<X, Y> o;
        if (useBase64) {
            xParser = Parsers::base64Decorator(xParser);
            yParser = Parsers::base64Decorator(yParser);
        }

        std::string x, y;
        while (r.read_row(x, y)) {
            o.add(xParser(x), yParser(y));
        }
        return o;
    }

    static FiniteRelation<X, Y> readFromCSV(std::function<X(std::string)> xParser, std::function<Y(std::string)> yParser, std::istream& is) {
        return readFromCSV(xParser, yParser, is, false);
    }

    static FiniteRelation<X, Y> readFromCSV(std::function<X(std::string)> xParser, std::function<Y(std::string)> yParser, const std::string& path, bool useBase64) {
        std::ifstream is(path);
        return readFromCSV(xParser, yParser, is, useBase64);
    }

    static FiniteRelation<X, Y> readFromCSV(std::function<X(std::string)> xParser, std::function<Y(std::string)> yParser, const std::string& path) {
        std::ifstream is(path);
        return readFromCSV(xParser, yParser, is, false);
    }

    std::string toString() const {
        return toString([](X t) { return std::to_string(t); }, [](Y u) { return std::to_string(u); });
    }

    std::string toString(std::function<std::string(X)> printer1, std::function<std::string(Y)> printer2) const {
        return toJSONArrayString(printer1, printer2);
    }

    bool apply(const X& a, const Y& b) const override {
        return pairs.find(HeteroPair<X, Y>(a, b)) != pairs.end();
    }

    int compareTo(const FiniteRelation<X, Y>& o) const {
        IterableComparator<HeteroPair<X, Y>> it;
        return it.compare(*this, o);
    }

    void printToJSON(std::function<std::string(X)> printer1, std::function<std::string(Y)> printer2, const std::string& path) const {
        toStringJaggedList(printer1, printer2).printToJSON([](const std::string& s) { return s; }, path);
    }

    std::string toJSONArrayString(std::function<std::string(X)> printer1, std::function<std::string(Y)> printer2) const {
        return toStringJaggedList(printer1, printer2).toJSONArrayString([](const std::string& s) { return s; });
    }

    static FiniteRelation<X, Y> parseJSONArray(const std::string& str, std::function<X(std::string)> parser1, std::function<Y(std::string)> parser2) {
        return fromStringJaggedList(JaggedList<std::string>::parseJSONArray(str, [](const std::string& s) { return s; }), parser1, parser2);
    }

    static FiniteRelation<X, Y> parseJSONFile(const std::string& path, std::function<X(std::string)> parser1, std::function<Y(std::string)> parser2) {
        return fromStringJaggedList(JaggedList<std::string>::parseJSONFile(path, [](const std::string& s) { return s; }), parser1, parser2);
    }

    JaggedList<std::string> toStringJaggedList(std::function<std::string(X)> printer1, std::function<std::string(Y)> printer2) const {
        JaggedList<std::string> o;
        for (const auto& p : pairs) {
            auto x = o.newChild();
            x.add(p.first == X() ? "null" : printer1(p.first));
            x.add(p.second == Y() ? "null" : printer2(p.second));
        }
        return o;
    }

    static FiniteRelation<X, Y> fromStringJaggedList(const JaggedList<std::string>& arr, std::function<X(std::string)> parser1, std::function<Y(std::string)> parser2) {
        FiniteRelation<X, Y> o;
        parser1 = Parsers::nullDecorator(parser1);
        parser2 = Parsers::nullDecorator(parser2);
        for (int i = 0; i < arr.size(); ++i) {
            o.add(parser1(arr.get(i, 0).value()), parser2(arr.get(i, 1).value()));
        }
        return o;
    }

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = HeteroPair<X, Y>;
        using difference_type = std::ptrdiff_t;
        using pointer = const HeteroPair<X, Y>*;
        using reference = const HeteroPair<X, Y>&;

        Iterator(typename std::set<HeteroPair<X, Y>>::const_iterator it) : it_(it) {}

        reference operator*() const { return *it_; }
        pointer operator->() const { return &(*it_); }

        Iterator& operator++() {
            ++it_;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++it_;
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) { return a.it_ == b.it_; }
        friend bool operator!=(const Iterator& a, const Iterator& b) { return a.it_ != b.it_; }

    private:
        typename std::set<HeteroPair<X, Y>>::const_iterator it_;
    };

    Iterator begin() const { return Iterator(pairs.begin()); }
    Iterator end() const { return Iterator(pairs.end()); }

private:
    std::set<HeteroPair<X, Y>> pairs;
    std::set<HeteroPair<Y, X>> pairsReversed;
    std::set<X> domain;
    std::set<Y> codomain;
};

