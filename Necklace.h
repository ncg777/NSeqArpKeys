#pragma once

#include <vector>
#include <set>
#include <stdexcept>
#include <algorithm>
#include "Sequence.h"

class Necklace : public Sequence {
private:
    int m_Order;
    static int cnt;

public:
    int getOrder() const {
        return m_Order;
    }

    Necklace(int k, int p_Order, const std::vector<int>& array) : Sequence(array) {
        bool ex = false;
        for (int i = 0; i < array.size(); ++i) {
            if (array[i] >= k || array[i] < 0) {
                ex = true;
                break;
            }
        }

        if (ex) {
            throw std::invalid_argument("Necklace constructor received incoherent arguments.");
        }
        m_Order = p_Order;
    }

    static std::set<Necklace> generate(int n, int k) {
        cnt = 0;
        std::set<Necklace> output;
        std::vector<int> a(n + 1, 0);
        subGen(1, 1, n, k, a, output);
        cnt = 0;
        return output;
    }

private:
    static void subGen(int t, int p, int n, int k, std::vector<int>& a, std::set<Necklace>& output) {
        if (t > n) {
            if ((n % p) == 0) {
                std::vector<int> tmp(n);
                for (int i = 0; i < n; ++i) {
                    tmp[i] = a[i + 1];
                }

                output.insert(Necklace(k, cnt, tmp));
                cnt++;
            }
        }
        else {
            a[t] = a[t - p];
            subGen(t + 1, p, n, k, a, output);
            for (int j = a[t - p] + 1; j < k; ++j) {
                a[t] = j;
                subGen(t + 1, t, n, k, a, output);
            }
        }
    }
};

int Necklace::cnt = 0;


