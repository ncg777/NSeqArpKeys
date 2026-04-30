#include "Combination.h"
#include "CollectionUtils.h"

Combination::Combination(const Combination& c) : std::bitset<128>(c), m_n(c.m_n) {}

Combination::Combination(int n) : m_n(n) {}
Combination::Combination() : m_n(0) {}
int Combination::find_first(const std::bitset<128>& bitset, int n) {
    for (int i = 0; i < n; ++i) {
        if (bitset.test(i)) {
            return i;
        }
    }
    return -1;
}

int Combination::find_next(const std::bitset<128>& bitset, int pos, int n) {
    for (int i = pos + 1; i < n; ++i) {
        if (bitset.test(i)) {
            return i;
        }
    }
    return -1;
}

int Combination::find_last(const std::bitset<128>& bitset, int n) {
    for (int i = n - 1; i >= 0; --i) {
        if (bitset.test(i)) {
            return i;
        }
    }
    return -1;
}

int Combination::find_first() const {
    for (int i = 0; i < m_n; ++i) {
        if (this->test(i)) {
            return i;
        }
    }
    return -1;
}

int Combination::find_next(int pos) const {
    for (int i = pos + 1; i < m_n; ++i) {
        if (this->test(i)) {
            return i;
        }
    }
    return -1;
}

int Combination::find_last() const {
    for (int i = m_n - 1; i >= 0; --i) {
        if (this->test(i)) {
            return i;
        }
    }
    return -1;
}

int Combination::getN() const {
    return m_n;
}

int Combination::getK() const {
    return this->count();
}

int Combination::calcSpan() const {
    return getN() - Composition::getCompositionFromCombination(*this).asSequence().getMax();
}

Sequence Combination::getIntervalVector() const {
    return CollectionUtils::calcIntervalVector(*this, m_n);
}

Combination::Combination(int n, const std::set<int>& s) : Combination(n) {
    for (int i = 0; i < n; ++i) {
        this->set(i, s.find(i) != s.end());
    }
}

Combination::Combination(const std::bitset<128>& c, int n) : std::bitset<128>(c), m_n(n) {}

Combination Combination::reverse() const {
    Combination o(this->getN());
    for (int i = 0; i < o.getN(); ++i) {
        if (this->test(i)) o.set(-1 + o.getN() - i);
    }
    return o;
}

double Combination::calcNormalizedDistanceWith(const Combination& other) const {
    int maxn = std::max(this->getN(), other.getN());
    int acc = 0;
    for (int i = 0; i < maxn; ++i) {
        if (this->test(i % this->getN()) != other.test(i % other.getN())) acc++;
    }
    return static_cast<double>(acc) / static_cast<double>(maxn);
}

Combination Combination::symmetricDifference(const Combination& y) const {
    int n = std::max(this->getN(), y.getN());
    Combination x = Combination(n);
    x |= *this;
    x ^= y;

    return x;
}

std::string Combination::toBinaryString() const {
    std::ostringstream oss;
    for (int i = 0; i < m_n; ++i) {
        oss << (this->test(i) ? "1" : "0");
    }
    return oss.str();
}

Combination Combination::fromBinaryString(const std::string& s) {
    std::string trimmed = s;
    trimmed.erase(trimmed.find_last_not_of(" \n\r\t") + 1);
    int n = trimmed.length();
    Combination o(n);
    for (int i = 0; i < n; ++i) {
        if (trimmed[i] != '0') o.set(i);
    }
    return o;
}

Combination Combination::mergeAll(const std::vector<Combination>& r) {
    Sequence sizes;
    for (const auto& comb : r) {
        sizes.push_back(comb.getN());
    }
    int max = sizes.getMax();
    int newsz = max * r.size();
    Combination b(newsz);
    Sequence idxs(r.size(), 0);
    for (int i = 0; i < newsz; ++i) {
        int wi = i % r.size();
        const auto& w = r[wi];
        b.set(i, w.test(idxs[wi]));
        idxs[wi] = (idxs[wi] + 1) % sizes[wi];
    }
    return b;
}

std::string Combination::toString() const {
    std::ostringstream oss;
    oss << "{";
    for (int i = this->find_first(); i >= 0; i = this->find_next(i)) {
        oss << i;
        if (i != this->find_last()) {
            oss << ", ";
        }
    }
    oss << "}";
    return oss.str();
}

Sequence Combination::asSequence() const {
    Sequence o;
    for (int i = this->find_first(); i >= 0; i = this->find_next(i + 1)) {
        o.push_back(i);
    }
    return o;
}

std::set<int> Combination::asSet() const {
    std::set<int> o;
    for (int i = this->find_first(); i >= 0; i = this->find_next(i + 1)) {
        o.insert(i);
    }
    return o;
}

Sequence Combination::asBinarySequence() const {
    Sequence o;
    for (int i = 0; i < getN(); ++i) {
        o.push_back(this->test(i) ? 1 : 0);
    }
    return o;
}

Combination Combination::fromBinarySequence(const Sequence& s) {
    Combination c(s.size());
    for (int i = 0; i < s.size(); ++i) {
        if (s[i] != 0) c.set(i, true);
    }
    return c;
}

int Combination::compareTo(const Combination& o) const {
    if (this->m_n < o.m_n) {
        return -1;
    }
    if (this->m_n > o.m_n) {
        return 1;
    }
    std::bitset<128> a;
    a |= *this;
    std::bitset<128> b;
    b |= o;
    a ^= b;
    int i = find_first(a, this->m_n);
    if (i == -1) {
        return 0;
    }
    else {
        return b.test(i) ? -1 : 1;
    }
}
bool Combination::operator<(const Combination& other) const {
    return compareTo(other) < 0;
}
std::vector<Combination> Combination::refinements(const Combination& c) {
    int n = c.getN() - c.getK();
    if (n == 0) {
        return {};
    }
    std::vector<Combination> o;
    int k = 0;
    for (int i = 0; i < n; ++i) {
        while (c.test(k)) {
            k++;
        }
        Combination b = Combination(c.getN());
        b |= c;
        b.set(k++);
        o.push_back(b);
    }
    return o;
}

Combination Combination::merge(const Combination& a, const Combination& b) {
    Combination x = Combination(std::max(a.getN(), b.getN()));
    x |= a;
    x |= b;
    return x;
}

Combination Combination::rotate(int t) const {
    int k = -t;
    while (k < 0) {
        k += m_n;
    }
    while (k >= m_n) {
        k -= m_n;
    }
    Combination x = Combination(this->getN());
    for (int i = 0; i < m_n; ++i) {
        x.set(i, this->test((i - k + m_n) % m_n));
    }
    return x;
}

Combination Combination::intersect(const Combination& c) const {
    int n = std::min(this->getN(), c.getN());
    Combination b = Combination(n);
    b |= *this;
    b &= c;
    return b;
}

Combination Combination::minus(const Combination& c) const {
    Combination o(*this);
    int n = std::min(getN(), c.getN());
    for (int i = 0; i < n; ++i) {
        if (o.test(i) && c.test(i)) {
            o.set(i, false);
        }
    }
    return o;
}

std::vector<Combination> Combination::partition(const Sequence& p0) const {
    std::vector<int> p(p0.begin(), p0.end());
    return partition(p);
}

std::vector<Combination> Combination::partition(const std::vector<int>& partition) const {
    if (partition.size() != this->getK()) {
        throw std::invalid_argument("Partition size does not match combination size");
    }
    int min = *std::min_element(partition.begin(), partition.end());
    int max = *std::max_element(partition.begin(), partition.end());
    if (min != 0 || max > getK()) {
        throw std::invalid_argument("Invalid partition values");
    }
    std::vector<int> set(this->getK());
    int k = 0;
    for (int i = this->find_first(); i > -1; i = this->find_next(i + 1)) {
        set[k++] = i;
    }
    std::vector<Combination> o(max + 1);
    std::vector<Combination> b(max + 1);
    for (int i = 0; i <= max; ++i) {
        b[i] = Combination(getN());
        for (int j = 0; j < partition.size(); ++j) {
            if (partition[j] == i) {
                b[i].set(set[j]);
            }
        }
        o[i] = Combination(b[i]);
    }
    return o;
}

Combination Combination::genRnd(int n) {
    return genRnd(n, RandomNumberGenerator::nextInt(n + 1));
}

Combination Combination::genRnd(int n, int k) {
    Combination o(n);
    int c = 0;
    int i = 0;
    double d;
    while (c < k) {
        d = RandomNumberGenerator::nextDouble();
        if (d < static_cast<double>(k - c) / static_cast<double>(n - i)) {
            o.set(i, true);
            c++;
        }
        i++;
    }
    return o;
}
