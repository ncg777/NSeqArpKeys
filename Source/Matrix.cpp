#include "Matrix.h"

template<typename T>
inline Matrix<T>::Matrix() : m(0), n(0), defaultValue(T()) {}

template<typename T>
inline Matrix<T>::Matrix(int rows, int cols) : m(rows), n(cols), defaultValue(T()) {}

template<typename T>
inline Matrix<T>::Matrix(int rows, int cols, T fill) : m(rows), n(cols), defaultValue(fill) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            set(i, j, fill);
        }
    }
}

template<typename T>
inline Matrix<T>::Matrix(const std::vector<std::vector<T>>& arr) : m(arr.size()), n(arr[0].size()), defaultValue(T()) {
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            set(i, j, arr[i][j]);
        }
    }
}

template<typename T>
inline Matrix<T>::Matrix(const Matrix<T>& other) : m(other.m), n(other.n), defaultValue(other.defaultValue) {
    mat = other.mat;
}

template<typename T>
inline int Matrix<T>::rowCount() const {
    return m;
}

template<typename T>
inline int Matrix<T>::columnCount() const {
    return n;
}

template<typename T>
inline void Matrix<T>::apply(const std::function<T(T)>& transform) {
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            set(i, j, transform(get(i, j)));
        }
    }
}

template<typename T>
inline T Matrix<T>::get(int i, int j) const {
    HomoPair<int> p(i, j);
    auto it = mat.find(p);
    if (it == mat.end()) {
        return defaultValue;
    }
    return it->second;
}

template<typename T>
inline void Matrix<T>::set(int i, int j, T val) {
    HomoPair<int> p(i, j);
    if (val == defaultValue) {
        mat.erase(p);
    }
    else {
        mat[p] = val;
    }
}

template<typename T>
inline void Matrix<T>::clear() {
    mat.clear();
}

template<typename T>
inline void Matrix<T>::appendRow(const std::vector<T>& row) {
    insertRow(m, row);
}

template<typename T>
inline void Matrix<T>::appendRow(T val) {
    insertRow(m, val);
}

template<typename T>
inline void Matrix<T>::appendColumn(const std::vector<T>& col) {
    insertColumn(n, col);
}

template<typename T>
inline void Matrix<T>::appendColumn(T val) {
    insertColumn(n, val);
}

template<typename T>
inline void Matrix<T>::insertRow(int i, const std::vector<T>& row) {
    if (row.size() != n && n != 0) {
        throw std::invalid_argument("Matrix::insertRow list size doesn't match matrix.");
    }
    shiftRowsDown(i);
    for (int j = 0; j < n; ++j) {
        set(i, j, row[j]);
    }
}

template<typename T>
inline void Matrix<T>::insertRow(int i, T val) {
    shiftRowsDown(i);
    for (int j = 0; j < n; ++j) {
        set(i, j, val);
    }
}

template<typename T>
inline void Matrix<T>::insertColumn(int j, const std::vector<T>& col) {
    if (col.size() != m && m != 0) {
        throw std::invalid_argument("Matrix::insertColumn list size doesn't match matrix.");
    }
    shiftColumnsRight(j);
    for (int i = 0; i < m; ++i) {
        set(i, j, col[i]);
    }
}

template<typename T>
inline void Matrix<T>::insertColumn(int j, T val) {
    shiftColumnsRight(j);
    for (int i = 0; i < m; ++i) {
        set(i, j, val);
    }
}

template<typename T>
inline void Matrix<T>::removeRow(int i) {
    m--;
    for (auto it = mat.begin(); it != mat.end();) {
        if (it->first.getFirst() == i) {
            it = mat.erase(it);
        }
        else {
            if (it->first.getFirst() > i) {
                HomoPair<int> newKey(it->first.getFirst() - 1, it->first.getSecond());
                mat[newKey] = it->second;
                it = mat.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

template<typename T>
inline void Matrix<T>::removeColumn(int j) {
    n--;
    for (auto it = mat.begin(); it != mat.end();) {
        if (it->first.getSecond() == j) {
            it = mat.erase(it);
        }
        else {
            if (it->first.getSecond() > j) {
                HomoPair<int> newKey(it->first.getFirst(), it->first.getSecond() - 1);
                mat[newKey] = it->second;
                it = mat.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

template<typename T>
inline std::vector<T> Matrix<T>::getRow(int i) const {
    std::vector<T> row(n);
    for (int j = 0; j < n; ++j) {
        row[j] = get(i, j);
    }
    return row;
}

template<typename T>
inline std::vector<T> Matrix<T>::getColumn(int j) const {
    std::vector<T> col(m);
    for (int i = 0; i < m; ++i) {
        col[i] = get(i, j);
    }
    return col;
}

template<typename T>
inline Matrix<T> Matrix<T>::getTranspose() const {
    Matrix<T> transposed(n, m);
    for (const auto& entry : mat) {
        transposed.set(entry.first.getSecond(), entry.first.getFirst(), entry.second);
    }
    return transposed;
}

template<typename T>
inline Matrix<T> Matrix<T>::product(const Matrix<T>& other, T initValue, const std::function<T(T, T)>& sum, const std::function<T(T, T)>& product) const {
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

template<typename T>
inline Matrix<T> Matrix<T>::kronecker(const Matrix<T>& other, const std::function<T(T, T)>& product) const {
    int resultRows = m * other.m;
    int resultCols = n * other.n;

    Matrix<T> result(resultRows, resultCols);

    MixedRadixEnumeration mre({ m, n, other.m, other.n });
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

template<typename T>
inline std::string Matrix<T>::toString() const {
    return toString([](T t) { return t == T() ? "?" : std::to_string(t); });
}

template<typename T>
inline std::string Matrix<T>::toString(const std::function<std::string(T)>& printer) const {
    std::ostringstream oss;
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            oss << printer(get(i, j)) << " ";
        }
        oss << "\n";
    }
    return oss.str();
}

template<typename T>
inline bool Matrix<T>::contains(T el) const {
    if (el == defaultValue && m > 0 && n > 0) return true;
    return std::any_of(mat.begin(), mat.end(), [el](const auto& entry) { return entry.second == el; });
}

template<typename T>
inline bool Matrix<T>::operator==(const Matrix<T>& other) const {
    return m == other.m && n == other.n && mat == other.mat;
}

template<typename T>
inline bool Matrix<T>::operator!=(const Matrix<T>& other) const {
    return !(*this == other);
}
template<typename T>
inline bool Matrix<T>::operator<(const Matrix<T>& other) const {
    if (m != other.m) return m < other.m;
    if (n != other.n) return n < other.n;
    return mat < other.mat;
}
template<typename T>
inline int Matrix<T>::compareTo(const Matrix<T>& other) const {
    if (*this < other) return -1;
    if (*this == other) return 0;
    return 1;
}

template<typename T>
inline void Matrix<T>::shiftColumnsRight(int j) {
    n++;
    for (auto it = mat.rbegin(); it != mat.rend(); ++it) {
        if (it->first.getSecond() >= j) {
            HomoPair<int> newKey(it->first.getFirst(), it->first.getSecond() + 1);
            mat[newKey] = it->second;
        }
    }
}

template<typename T>
inline void Matrix<T>::shiftRowsDown(int i) {
    m++;
    for (auto it = mat.rbegin(); it != mat.rend(); ++it) {
        if (it->first.getFirst() >= i) {
            HomoPair<int> newKey(it->first.getFirst() + 1, it->first.getSecond());
            mat[newKey] = it->second;
        }
    }
}

template<typename T>
template<typename U>
inline Matrix<U> Matrix<T>::map(const std::function<U(T)>& transformer) const {
    Matrix<U> o(m, n, transformer(defaultValue));
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            o.set(i, j, transformer(get(i, j)));
        }
    }
    return o;
}
