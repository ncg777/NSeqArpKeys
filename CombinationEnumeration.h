#pragma once

#include <optional>
#include "Combination.h"

class CombinationEnumeration {
public:
    CombinationEnumeration(int n, int k) : current(first(n, k)) {}

    bool hasMoreElements() const {
        return current.has_value();
    }

    Combination nextElement() {
        if (!current.has_value()) {
            throw std::out_of_range("No more elements.");
        }
        Combination o = current.value();
        current = next(current.value());
        return o;
    }

private:
    std::optional<Combination> current;

    static std::optional<Combination> first(int n, int k) {
        if (k > n || n < 0 || k < 0) {
            return std::nullopt;
        }
        Combination o(n);

        for (int i = 0; i < k; ++i) {
            o.set(i, true);
        }
        return o;
    }

    static std::optional<Combination> next(const Combination& c) {
        int n = c.getN();
        std::optional<Combination> o = std::nullopt;

        int j = -1;
        for (int i = 0; i < n - 1; ++i) {
            if (c.test(i) && !c.test(i + 1)) {
                j = i;
                break;
            }
        }
        if (j != -1) {
            o = Combination(c);
            o->set(j, false);
            o->set(j + 1, true);
            int s = -1;
            for (int i = 0; i < j; ++i) {
                if (!o->test(i)) {
                    s = i;
                    break;
                }
            }

            for (int i = j; i >= 0; --i) {
                if (o->test(i) && s != -1 && s < i) {
                    o->set(i, false);
                    o->set(s, true);
                    while (s < j && o->test(s)) {
                        s++;
                    }
                }
            }
        }
        return o;
    }
};

