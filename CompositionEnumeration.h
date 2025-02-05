#pragma once

#include "BitSetEnumeration.h"
#include "Composition.h"

template <std::size_t N>
class CompositionEnumeration {
public:
    CompositionEnumeration(int n) : be(n - 1), n(n) {
        if (n < 1) {
            throw std::invalid_argument("n must be greater than 0.");
        }
    }

    bool hasMoreElements() const {
        return be.hasMoreElements();
    }

    Composition nextElement() {
        return Composition(be.nextElement(), n);
    }

private:
    BitSetEnumeration<N> be;
    int n;
};

