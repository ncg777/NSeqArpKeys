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
    Matrix();

    Matrix(int rows, int cols);

    Matrix(int rows, int cols, T fill);

    Matrix(const std::vector<std::vector<T>>& arr);

    Matrix(const Matrix<T>& other);

    int rowCount() const;

    int columnCount() const;

    void apply(const std::function<T(T)>& transform);

    T get(int i, int j) const;

    void set(int i, int j, T val);

    void clear();

    void appendRow(const std::vector<T>& row);

    void appendRow(T val);

    void appendColumn(const std::vector<T>& col);

    void appendColumn(T val);

    void insertRow(int i, const std::vector<T>& row);

    void insertRow(int i, T val);

    void insertColumn(int j, const std::vector<T>& col);

    void insertColumn(int j, T val);

    void removeRow(int i);

    void removeColumn(int j);

    std::vector<T> getRow(int i) const;

    std::vector<T> getColumn(int j) const;

    Matrix<T> getTranspose() const;

    Matrix<T> product(const Matrix<T>& other, T initValue, const std::function<T(T, T)>& sum, const std::function<T(T, T)>& product) const;

    Matrix<T> kronecker(const Matrix<T>& other, const std::function<T(T, T)>& product) const;

    std::string toString() const;

    std::string toString(const std::function<std::string(T)>& printer) const;

    bool contains(T el) const;

    bool operator==(const Matrix<T>& other) const;

    bool operator!=(const Matrix<T>& other) const;

    bool operator<(const Matrix<T>& other) const;

    int compareTo(const Matrix<T>& other) const;

    template <typename U>
    Matrix<U> map(const std::function<U(T)>& transformer) const;


private:
    void shiftColumnsRight(int j);

    void shiftRowsDown(int i);
};
