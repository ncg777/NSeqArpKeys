#pragma once

#include <map>
#include <set>
#include <vector>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <algorithm>
#include "HomoPair.h"
#include "Sequence.h"

template <typename T>
class Matrix {
protected:
    std::map<HomoPair<int>, T> mat;
    int m;
    int n;
    T defaultValue;

public:
    Matrix() : m(0), n(0), defaultValue(T()) {}

    Matrix(int rows, int cols) : m(rows), n(cols), defaultValue(T()) {}

    Matrix(int rows, int cols, T fill) : m(rows), n(cols), defaultValue(fill) {
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                set(i, j, fill);
            }
        }
    }

    Matrix(const std::vector<std::vector<T>>& arr) : m(arr.size()), n(arr[0].size()), defaultValue(T()) {
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j) {
                set(i, j, arr[i][j]);
            }
        }
    }

    Matrix(const Matrix<T>& other) : m(other.m), n(other.n), defaultValue(other.defaultValue) {
        mat = other.mat;
    }

    int rowCount() const {
        return m;
    }

    int columnCount() const {
        return n;
    }

    void apply(const std::function<T(T)>& transform) {
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j) {
                set(i, j, transform(get(i, j)));
            }
        }
    }

    T get(int i, int j) const {
        HomoPair<int> p(i, j);
        auto it = mat.find(p);
        if (it == mat.end()) {
            return defaultValue;
        }
        return it->second;
    }

    void set(int i, int j, T val) {
        HomoPair<int> p(i, j);
        if (val == defaultValue) {
            mat.erase(p);
        } else {
            mat[p] = val;
        }
    }

    void clear() {
        mat.clear();
    }

    void appendRow(const std::vector<T>& row) {
        insertRow(m, row);
    }

    void appendRow(T val) {
        insertRow(m, val);
    }

    void appendColumn(const std::vector<T>& col) {
        insertColumn(n, col);
    }

    void appendColumn(T val) {
        insertColumn(n, val);
    }

    void insertRow(int i, const std::vector<T>& row) {
        if (row.size() != n && n != 0) {
            throw std::invalid_argument("Matrix::insertRow list size doesn't match matrix.");
        }
        shiftRowsDown(i);
        for (int j = 0; j < n; ++j) {
            set(i, j, row[j]);
        }
    }

    void insertRow(int i, T val) {
        shiftRowsDown(i);
        for (int j = 0; j < n; ++j) {
            set(i, j, val);
        }
    }

    void insertColumn(int j, const std::vector<T>& col) {
        if (col.size() != m && m != 0) {
            throw std::invalid_argument("Matrix::insertColumn list size doesn't match matrix.");
        }
        shiftColumnsRight(j);
        for (int i = 0; i < m; ++i) {
            set(i, j, col[i]);
        }
    }

    void insertColumn(int j, T val) {
        shiftColumnsRight(j);
        for (int i = 0; i < m; ++i) {
            set(i, j, val);
        }
    }

    void removeRow(int i) {
        m--;
        for (auto it = mat.begin(); it != mat.end();) {
            if (it->first.getFirst() == i) {
                it = mat.erase(it);
            } else {
                if (it->first.getFirst() > i) {
                    HomoPair<int> newKey(it->first.getFirst() - 1, it->first.getSecond());
                    mat[newKey] = it->second;
                    it = mat.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    void removeColumn(int j) {
        n--;
        for (auto it = mat.begin(); it != mat.end();) {
            if (it->first.getSecond() == j) {
                it = mat.erase(it);
            } else {
                if (it->first.getSecond() > j) {
                    HomoPair<int> newKey(it->first.getFirst(), it->first.getSecond() - 1);
                    mat[newKey] = it->second;
                    it = mat.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    std::vector<T> getRow(int i) const {
        std::vector<T> row(n);
        for (int j = 0; j < n; ++j) {
            row[j] = get(i, j);
        }
        return row;
    }

    std::vector<T> getColumn(int j) const {
        std::vector<T> col(m);
        for (int i = 0; i < m; ++i) {
            col[i] = get(i, j);
        }
        return col;
    }

    Matrix<T> getTranspose() const {
        Matrix<T> transposed(n, m);
        for (const auto& entry : mat) {
            transposed.set(entry.first.getSecond(), entry.first.getFirst(), entry.second);
        }
        return transposed;
    }

    Matrix<T> product(const Matrix<T>& other, T initValue, const std::function<T(T, T)>& sum, const std::function<T(T, T)>& product) const {
        if (n != other.m) {
            throw std::invalid_argument("Matrix dimensions do not match for multiplication");
        }

        Matrix<T> result(m, other.n);

        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < other.n; ++j) {
                T total = initValue;
                for (int k = 0; k < n; ++k) {
                    total = sum(total, product(get(i, k), other.get(k, j)));
                }
                result.set(i, j, total);
            }
        }

        return result;
    }

    Matrix<T> kronecker(const Matrix<T>& other, const std::function<T(T, T)>& product) const {
        int resultRows = m * other.m;
        int resultCols = n * other.n;

        Matrix<T> result(resultRows, resultCols);

        MixedRadixEnumeration mre({m, n, other.m, other.n});
        std::set<Sequence> t;
        while (mre.hasMoreElements()) {
            t.insert(Sequence(mre.nextElement()));
        }

        for (const auto& s : t) {
            int i = s[0];
            int j = s[1];
            int k = s[2];
            int l = s[3];

            T value = product(get(i, j), other.get(k, l));

            result.set((i * other.m) + k, (j * other.n) + l, value);
        }

        return result;
    }

    std::string toString() const {
        return toString([](T t) { return t == T() ? "?" : std::to_string(t); });
    }

    std::string toString(const std::function<std::string(T)>& printer) const {
        std::ostringstream oss;
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j) {
                oss << printer(get(i, j)) << " ";
            }
            oss << "\n";
        }
        return oss.str();
    }

    bool contains(T el) const {
        if (el == defaultValue && m > 0 && n > 0) return true;
        return std::any_of(mat.begin(), mat.end(), [el](const auto& entry) { return entry.second == el; });
    }

    bool operator==(const Matrix<T>& other) const {
        return m == other.m && n == other.n && mat == other.mat;
    }

    bool operator!=(const Matrix<T>& other) const {
        return !(*this == other);
    }

    bool operator<(const Matrix<T>& other) const {
        if (m != other.m) return m < other.m;
        if (n != other.n) return n < other.n;
        return mat < other.mat;
    }

    int compareTo(const Matrix<T>& other) const {
        if (*this < other) return -1;
        if (*this == other) return 0;
        return 1;
    }

    template <typename U>
    Matrix<U> map(const std::function<U(T)>& transformer) const {
        Matrix<U> o(m, n, transformer(defaultValue));
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j) {
                o.set(i, j, transformer(get(i, j)));
            }
        }
        return o;
    }


private:
    void shiftColumnsRight(int j) {
        n++;
        for (auto it = mat.rbegin(); it != mat.rend(); ++it) {
            if (it->first.getSecond() >= j) {
                HomoPair<int> newKey(it->first.getFirst(), it->first.getSecond() + 1);
                mat[newKey] = it->second;
            }
        }
    }

    void shiftRowsDown(int i) {
        m++;
        for (auto it = mat.rbegin(); it != mat.rend(); ++it) {
            if (it->first.getFirst() >= i) {
                HomoPair<int> newKey(it->first.getFirst() + 1, it->first.getSecond());
                mat[newKey] = it->second;
            }
        }
    }
};

