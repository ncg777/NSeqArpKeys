#pragma once

#include <cmath>
#include <stdexcept>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include "Matrix.h"

class Numbers {
public:
    static bool divides(int k, int n);

    static std::set<int> factors(int n);

    static long qFactorial(int q, int n);

    static long qBinomial(int q, int n, int k);

    static long repunitBin(int n);

    static bool isPowerOfTwo(int n);

    static int minDistMod12(int a, int b);

    static int correctMod(int a, int b);

    static bool prime(int n0);

    static long gcd(long a0, long b0);

    static long lcm(long a, long b);

    static long catalan(int n);

    static long bell(int n);

    static long binomial(int n, int k);

    static long multinomial(const std::vector<int>& n);

    static long factorial(int n);

    static long triangularNumber(int n);

    static long reverseTriangularNumber(int n);
};
