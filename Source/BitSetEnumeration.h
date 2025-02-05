#pragma once

#include <bitset>
#include <stdexcept>
#include <optional>

template <std::size_t N>
class BitSetEnumeration {
public:
    BitSetEnumeration() : current(std::bitset<N>()), n(N) {
        if (n < 0) {
            throw std::invalid_argument("n must be non-negative.");
        }
    }

    bool hasMoreElements() const {
        return current.has_value();
    }

    std::bitset<N> nextElement() {
        if (!current.has_value()) {
            throw std::out_of_range("No more elements.");
        }
        std::bitset<N> o = current.value();
        current = next(current.value());
        return o;
    }

private:
    std::optional<std::bitset<N>> current;
    int n;

    std::optional<std::bitset<N>> next(const std::bitset<N>& b) {
        std::bitset<N> o = b;
        bool isLast = true;
        for (int i = 0; i < n; ++i) {
            if (!o.test(i)) {
                o.set(i, true);
                isLast = false;
                break;
            }
            else {
                o.set(i, false);
            }
        }

        if (isLast) {
            return std::nullopt;
        }

        return o;
    }
};

