#pragma once

#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>
#include <memory>
#include <algorithm>
#include <iterator>
#include <type_traits>

template <typename T>
class JaggedList {
public:
    using Ptr = std::shared_ptr<JaggedList<T>>;

private:
    T value;
    Ptr parent;
    std::vector<Ptr> children;

public:
    JaggedList() : value(T()), parent(nullptr) {}

    JaggedList(T t) : value(t), parent(nullptr) {}

    JaggedList(int dimensions) {
        init(dimensions);
    }

    int size() const {
        return children.size();
    }

    void init(int dimensions) {
        init(T(), dimensions);
    }

    void init(T fillValue, int dimensions) {
        if (dimensions == 0) {
            setValue(fillValue);
            return;
        }

        for (int i = 0; i < dimensions; ++i) {
            newChild();
        }
        for (auto& c : children) {
            c->init(fillValue, dimensions - 1);
        }
    }

    bool isValue() const {
        return children.empty();
    }

    Ptr set(int index, T element) {
        children[index]->parent = nullptr;
        children[index] = std::make_shared<JaggedList<T>>(element, this->shared_from_this());
        return children[index];
    }

    Ptr set(int index, Ptr element) {
        element->parent = this->shared_from_this();
        auto current = children[index];
        if (current == element) return current;
        current->parent = nullptr;
        children[index] = element;
        return current;
    }

    bool add(T element) {
        auto c = newChild();
        return c->setValue(element);
    }

    Ptr newChild() {
        auto o = std::make_shared<JaggedList<T>>();
        addChild(o);
        return o;
    }

    bool addChild(Ptr t) {
        t->parent = this->shared_from_this();
        value = T();
        children.push_back(t);
        return true;
    }

    Ptr get(int index) const {
        return children[index];
    }

    T getValue(int index) const {
        return get(index)->getValue();
    }

    bool set(T value, int index) {
        return get(index)->setValue(value);
    }

    JaggedList(T t, Ptr parent) : value(t), parent(parent) {
        if (!parent) throw std::runtime_error("parent is null.");
        parent->addChild(this->shared_from_this());
    }

    bool setValue(T t) {
        bool changed = (value != t);
        value = t;
        children.clear();
        return changed;
    }

    T getValue() const {
        if (!isValue()) throw std::runtime_error("is not value");
        return value;
    }

    bool isRoot() const {
        return !parent;
    }

    Ptr getParent() const {
        return parent;
    }

    int getDepth() const {
        return parent ? parent->getDepth() + 1 : 0;
    }

    int getHeight() const {
        if (isValue()) return 0;
        int maxHeight = 0;
        for (const auto& c : children) {
            int h = c->getHeight();
            if (h > maxHeight) maxHeight = h;
        }
        return maxHeight + 1;
    }

    int getIndex() const {
        if (!parent) return -1;
        auto it = std::find(parent->children.begin(), parent->children.end(), this->shared_from_this());
        if (it == parent->children.end()) throw std::runtime_error("not found?");
        return std::distance(parent->children.begin(), it);
    }

    std::vector<int> getCoordinates() const {
        int depth = getDepth();
        std::vector<int> coords(depth);
        auto current = this->shared_from_this();
        for (int i = depth - 1; i >= 0; --i) {
            coords[i] = current->getIndex();
            current = current->getParent();
        }
        return coords;
    }

    Ptr getRoot() const {
        auto current = this->shared_from_this();
        while (!current->isRoot()) {
            current = current->getParent();
        }
        return current;
    }

    int compareTo(const JaggedList<T>& other) const {
        int cmp = (value < other.value) ? -1 : (value > other.value) ? 1 : 0;
        if (cmp == 0) {
            return std::lexicographical_compare(children.begin(), children.end(), other.children.begin(), other.children.end(),
                [](const Ptr& a, const Ptr& b) { return a->compareTo(*b); }) ? -1 : 1;
        }
        return cmp;
    }

    bool operator==(const JaggedList<T>& other) const {
        return compareTo(other) == 0;
    }

    bool operator!=(const JaggedList<T>& other) const {
        return !(*this == other);
    }

    bool operator<(const JaggedList<T>& other) const {
        return compareTo(other) < 0;
    }

    std::size_t hash() const {
        std::size_t seed = 0;
        seed ^= std::hash<T>()(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        for (const auto& child : children) {
            seed ^= child->hash() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }

    struct Hash {
        std::size_t operator()(const JaggedList<T>& jl) const {
            return jl.hash();
        }
    };

    struct Equal {
        bool operator()(const JaggedList<T>& a, const JaggedList<T>& b) const {
            return a == b;
        }
    };

    struct Less {
        bool operator()(const JaggedList<T>& a, const JaggedList<T>& b) const {
            return a < b;
        }
    };

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Ptr;
        using difference_type = std::ptrdiff_t;
        using pointer = Ptr*;
        using reference = Ptr&;

        Iterator() : current(nullptr) {}

        Iterator(Ptr ptr) : current(ptr) {}

        reference operator*() {
            return current;
        }

        pointer operator->() {
            return &current;
        }

        Iterator& operator++() {
            if (current) {
                current = current->parent;
            }
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& other) const {
            return current == other.current;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        Ptr current;
    };

    Iterator begin() {
        return Iterator(this->shared_from_this());
    }

    Iterator end() {
        return Iterator(nullptr);
    }
};


