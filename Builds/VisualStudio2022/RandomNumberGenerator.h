#pragma once

#include <random>
#include <chrono>

class RandomNumberGenerator {
public:
    static double nextDouble() {
        return getInstance().distribution(getInstance().engine);
    }

    static int nextInt() {
        return getInstance().intDistribution(getInstance().engine);
    }

    static int nextInt(int n) {
        std::uniform_int_distribution<int> dist(0, n - 1);
        return dist(getInstance().engine);
    }

    static double nextGaussian() {
        return getInstance().normalDistribution(getInstance().engine);
    }

private:
    RandomNumberGenerator()
        : engine(static_cast<unsigned long>(std::chrono::system_clock::now().time_since_epoch().count())),
        distribution(0.0, 1.0),
        normalDistribution(0.0, 1.0),
        intDistribution(0, std::numeric_limits<int>::max()) {
    }

    static RandomNumberGenerator& getInstance() {
        static RandomNumberGenerator instance;
        return instance;
    }

    std::mt19937 engine;
    std::uniform_real_distribution<double> distribution;
    std::normal_distribution<double> normalDistribution;
    std::uniform_int_distribution<int> intDistribution;
};

