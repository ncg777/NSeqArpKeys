#include "Numbers.h"

inline bool Numbers::divides(int k, int n) {
    return n % k == 0;
}

inline std::set<int> Numbers::factors(int n) {
    if (n < 1) throw std::runtime_error("factors:: invalid n");
    std::set<int> o;
    o.insert(1);
    o.insert(n);
    int u = static_cast<int>(std::floor(std::sqrt(n)));

    for (int i = 2; i <= u; ++i) {
        if (divides(i, n)) {
            o.insert(i);
            o.insert(n / i);
        }
    }
    return o;
}

inline long Numbers::qFactorial(int q, int n) {
    if (n < 0 || q <= 0) {
        throw std::invalid_argument("n must be non-negative and q must be positive.");
    }

    if (q == 1) {
        return factorial(n);
    }

    long qFactorial = 1;

    for (int i = 1; i <= n; ++i) {
        long qPowerI = static_cast<long>(std::pow(q, i));
        long numerator = qPowerI - 1;
        long denominator = q - 1;
        long qInteger = numerator / denominator;

        if (qFactorial > LONG_MAX / qInteger) {
            throw std::overflow_error("q-factorial overflows a long for n = " + std::to_string(n) + ", q = " + std::to_string(q));
        }

        qFactorial *= qInteger;
    }

    return qFactorial;
}

inline long Numbers::qBinomial(int q, int n, int k) {
    if (n < 0 || k < 0 || k > n || q <= 0) {
        throw std::invalid_argument("Invalid inputs: ensure n >= 0, 0 <= k <= n, and q > 0.");
    }

    long nFactorial = qFactorial(q, n);
    long kFactorial = qFactorial(q, k);
    long nMinusKFactorial = qFactorial(q, n - k);

    if (kFactorial > 0 && nMinusKFactorial > 0 && nFactorial % (kFactorial * nMinusKFactorial) == 0) {
        return nFactorial / (kFactorial * nMinusKFactorial);
    }
    else {
        throw std::overflow_error("Gaussian binomial coefficient computation failed due to overflow or division error.");
    }
}

inline long Numbers::repunitBin(int n) {
    return qBinomial(2, n, 1);
}

inline bool Numbers::isPowerOfTwo(int n) {
    return (static_cast<int>(std::round(std::pow(2.0, std::round(std::log(n) / std::log(2.0))))) == n);
}

inline int Numbers::minDistMod12(int a, int b) {
    int d1 = a - b;
    if (d1 < 0) d1 += 12;
    int d2 = b - a;
    if (d2 < 0) d2 += 12;
    return std::min(d1, d2);
}

inline int Numbers::correctMod(int a, int b) {
    if (b < 0) throw std::runtime_error("Natural.CorrectMod: invalid parameters.");
    if (a >= 0) return a % b;

    int a0 = a;
    while (a0 < 0) a0 += b;
    return a0 % b;
}

inline bool Numbers::prime(int n0) {
    int n = std::abs(n0);
    if (n < 2) {
        return false;
    }
    int s = static_cast<int>(std::floor(std::sqrt(n)));
    for (int i = 2; i <= s; ++i) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

inline long Numbers::gcd(long a0, long b0) {
    long a = a0;
    long b = b0;
    long t = 0;
    while (b != 0) {
        t = b;
        b = a % b;
        a = t;
    }
    return a;
}

inline long Numbers::lcm(long a, long b) {
    return (a * b) / gcd(a, b);
}

inline long Numbers::catalan(int n) {
    if (n < 0) {
        throw std::invalid_argument("n must be non-negative.");
    }

    std::vector<long> catalan(n + 1, 0);
    catalan[0] = 1;

    for (int i = 1; i <= n; ++i) {
        for (int j = 0; j < i; ++j) {
            if (catalan[i] > LONG_MAX / catalan[j]) {
                throw std::overflow_error("Overflow detected.");
            }

            catalan[i] += catalan[j] * catalan[i - 1 - j];

            if (catalan[i] < 0) {
                throw std::overflow_error("Overflow detected.");
            }
        }
    }

    return catalan[n];
}

inline long Numbers::bell(int n) {
    std::vector<std::vector<long>> bellTriangle(n + 1, std::vector<long>(n + 1, 0));

    bellTriangle[0][0] = 1;

    for (int i = 1; i <= n; ++i) {
        bellTriangle[i][0] = bellTriangle[i - 1][i - 1];

        for (int j = 1; j <= i; ++j) {
            bellTriangle[i][j] = bellTriangle[i][j - 1] + bellTriangle[i - 1][j - 1];
        }
    }

    return bellTriangle[n][0];
}

inline long Numbers::binomial(int n, int k) {
    if (n < 0) {
        throw std::invalid_argument("n must be non-negative.");
    }
    if (k < 0) {
        throw std::invalid_argument("k must be non-negative.");
    }
    if (k > n) {
        throw std::invalid_argument("k cannot be greater than n.");
    }

    if (k > n - k) {
        k = n - k;
    }

    long result = 1;

    for (int i = 0; i < k; ++i) {
        if (result > LONG_MAX / (n - i)) {
            throw std::overflow_error("Overflow detected.");
        }

        result *= (n - i);
        result /= (i + 1);
    }

    return result;
}

inline long Numbers::multinomial(const std::vector<int>& n) {
    if (n.empty()) {
        throw std::invalid_argument("Invalid input: n is empty.");
    }

    int sum = 0;
    for (int ni : n) {
        if (ni < 0) {
            throw std::invalid_argument("Invalid input: n contains negative values.");
        }
        sum += ni;
    }

    long nf = factorial(sum);
    for (int ni : n) {
        nf /= factorial(ni);
    }

    return nf;
}

inline long Numbers::factorial(int n) {
    if (n < 0) {
        throw std::invalid_argument("Invalid input: n must be non-negative.");
    }
    long o = 1;
    for (int i = 2; i <= n; ++i) {
        o *= i;
    }
    return o;
}

inline long Numbers::triangularNumber(int n) {
    return binomial(n + 1, 2);
}

inline long Numbers::reverseTriangularNumber(int n) {
    return static_cast<long>(std::floor((std::sqrt(1.0 + 8.0 * n) - 1.0) / 2.0));
}
