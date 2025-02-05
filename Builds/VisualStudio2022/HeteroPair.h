#pragma once

#include <string>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <fstream>

template <typename T, typename U>
class HeteroPair {
public:
    static HeteroPair<T, U> makeHeteroPair(const T& first, const U& second) {
        return HeteroPair<T, U>(first, second);
    }

    T getFirst() const {
        return x;
    }

    U getSecond() const {
        return y;
    }

    HeteroPair<U, T> converse() const {
        return makeHeteroPair(y, x);
    }

    bool operator==(const HeteroPair<T, U>& other) const {
        return compareTo(other) == 0;
    }

    bool operator!=(const HeteroPair<T, U>& other) const {
        return !(*this == other);
    }

    bool operator<(const HeteroPair<T, U>& other) const {
        return compareTo(other) < 0;
    }

    bool operator>(const HeteroPair<T, U>& other) const {
        return compareTo(other) > 0;
    }

    bool operator<=(const HeteroPair<T, U>& other) const {
        return compareTo(other) <= 0;
    }

    bool operator>=(const HeteroPair<T, U>& other) const {
        return compareTo(other) >= 0;
    }

    std::string toString() const {
        return toString([](const T& t) { return std::to_string(t); }, [](const U& u) { return std::to_string(u); });
    }

    std::string toString(const std::function<std::string(const T&)>& printer1, const std::function<std::string(const U&)>& printer2) const {
        return toJSONObjectString(printer1, printer2);
    }

private:
    T x;
    U y;

    HeteroPair(const T& p_x, const U& p_y) : x(p_x), y(p_y) {}

    int compareTo(const HeteroPair<T, U>& other) const {
        if (x < other.x) return -1;
        if (x > other.x) return 1;
        if (y < other.y) return -1;
        if (y > other.y) return 1;
        return 0;
    }
};
