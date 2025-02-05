#pragma once
#include "HeteroPair.h"

template <typename T>
class HomoPair : public HeteroPair<T, T> {
public:
    static HomoPair<T> makeHomoPair(const T& first, const T& second) {
        return HomoPair<T>(first, second);
    }

    static HomoPair<T> fromHeteroPair(const HeteroPair<T, T>& o) {
        return HomoPair<T>(o.getFirst(), o.getSecond());
    }

    bool operator==(const HomoPair<T>& other) const {
        return this->compareTo(other) == 0;
    }

    bool operator!=(const HomoPair<T>& other) const {
        return !(*this == other);
    }

private:
    HomoPair(const T& p_x, const T& p_y) : HeteroPair<T, T>(p_x, p_y) {}
};
