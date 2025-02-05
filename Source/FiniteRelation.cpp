#include "FiniteRelation.h"

bool apply(const X& a, const Y& b) const {
    return pairs.find(HeteroPair<X, Y>(a, b)) != pairs.end();
}

int compareTo(const FiniteRelation<X, Y>& o) const {
    IterableComparator<HeteroPair<X, Y>> it;
    return it.compare(*this, o);
}

std::string toString() const {
    return toString([](X t) { return std::to_string(t); }, [](Y u) { return std::to_string(u); });
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

FiniteRelation<X, Y> fromStringJaggedList(const JaggedList<std::string>& arr, std::function<X(std::string)> parser1, std::function<Y(std::string)> parser2) {
    FiniteRelation<X, Y> o;
    parser1 = Parsers::nullDecorator(parser1);
    parser2 = Parsers::nullDecorator(parser2);
    for (int i = 0; i < arr.size(); ++i) {
        o.add(parser1(arr.get(i, 0).value()), parser2(arr.get(i, 1).value()));
    }
    return o;
}

bool operator==(const Iterator& a, const Iterator& b) { return a.it_ == b.it_; }

bool operator!=(const Iterator& a, const Iterator& b) { return a.it_ != b.it_; }

Iterator begin() const { return Iterator(pairs.begin()); }

Iterator end() const { return Iterator(pairs.end()); }

inline Iterator::Iterator(typename std::set<HeteroPair<X, Y>>::const_iterator it) : it_(it) {}

inline reference Iterator::operator*() const { return *it_; }

inline pointer Iterator::operator->() const { return &(*it_); }

inline Iterator& Iterator::operator++() {
    ++it_;
    return *this;
}

inline Iterator Iterator::operator++(int) {
    Iterator tmp = *this;
    ++it_;
    return tmp;
}
