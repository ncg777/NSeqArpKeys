#pragma once

#include <iterator>
#include <vector>
#include <list>
#include <functional>
#include <algorithm>

template <typename T>
class IterableComparator {
public:
    using Comparator = std::function<int(const T&, const T&)>;

    IterableComparator() : comparator(nullptr) {}
    IterableComparator(Comparator comp) : comparator(comp) {}

    int compare(const std::vector<T>& arg0, const std::vector<T>& arg1) const {
        return compare(arg0.begin(), arg0.end(), arg1.begin(), arg1.end(), comparator);
    }

    int compare(const std::list<T>& arg0, const std::list<T>& arg1) const {
        return compare(arg0.begin(), arg0.end(), arg1.begin(), arg1.end(), comparator);
    }

    template <typename Iterator>
    static int compare(Iterator i, Iterator i_end, Iterator j, Iterator j_end, Comparator comparator = nullptr) {
        while (i != i_end && j != j_end) {
            const T& i_el = *i;
            const T& j_el = *j;

            if (i_el == nullptr && j_el == nullptr) {
                ++i;
                ++j;
                continue;
            }
            if (i_el == nullptr) {
                return -1;
            }
            if (j_el == nullptr) {
                return 1;
            }

            int c;
            if (comparator == nullptr) {
                c = (i_el < j_el) ? -1 : (i_el > j_el) ? 1 : 0;
            }
            else {
                c = comparator(i_el, j_el);
            }
            if (c != 0) {
                return c;
            }

            ++i;
            ++j;
        }

        if (i == i_end && j != j_end) {
            return -1;
        }
        if (j == j_end && i != i_end) {
            return 1;
        }

        return 0;
    }

    static int reverseCompare(const std::vector<T>& i, const std::vector<T>& j) {
        return reverseCompare(i, j, nullptr);
    }

    static int reverseCompare(const std::list<T>& i, const std::list<T>& j) {
        return reverseCompare(i, j, nullptr);
    }

    static int reverseCompare(const std::vector<T>& i, const std::vector<T>& j, Comparator comparator) {
        return reverseCompare(i.rbegin(), i.rend(), j.rbegin(), j.rend(), comparator);
    }

    static int reverseCompare(const std::list<T>& i, const std::list<T>& j, Comparator comparator) {
        return reverseCompare(i.rbegin(), i.rend(), j.rbegin(), j.rend(), comparator);
    }

private:
    Comparator comparator;

    template <typename ReverseIterator>
    static int reverseCompare(ReverseIterator i, ReverseIterator i_end, ReverseIterator j, ReverseIterator j_end, Comparator comparator = nullptr) {
        while (i != i_end && j != j_end) {
            int c;
            if (comparator == nullptr) {
                c = (*i < *j) ? -1 : (*i > *j) ? 1 : 0;
            }
            else {
                c = comparator(*i, *j);
            }
            if (c != 0) {
                return c;
            }

            ++i;
            ++j;
        }

        if (i == i_end && j != j_end) {
            return -1;
        }
        if (j == j_end && i != i_end) {
            return 1;
        }

        return 0;
    }
};

