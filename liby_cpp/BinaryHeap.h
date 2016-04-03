//
// Created by ccsexyz on 16-4-2.
//

#ifndef POLLERTEST_BINARYHEAP_H
#define POLLERTEST_BINARYHEAP_H

#include <initializer_list>
#include <stdint.h>
#include <vector>
#include <functional>

namespace Liby {
template <typename T> class BinaryHeap {
public:
    using size_type = uint32_t;
    using value_type = T;
    using reference_type = T &;
    using const_reference_type = const T &;

    explicit BinaryHeap(size_type capacity = 100) {
        heap.resize(capacity);
        size_ = 0;
    }

    explicit BinaryHeap(std::initializer_list<value_type> L)
        : heap(L.size() + 10), size_(0) {
        for (auto &x : L) {
            insert(x);
        }
    }

    explicit BinaryHeap(const std::vector<T> &items)
        : heap(items.size() + 10), size_(items.size()) {
        for (int i = 0; i < items.size(); i++)
            heap[i + 1] = items[i];
        build();
    }

    bool empty() const { return size_ == 0; }

    size_type size() const { return size_; }

    const_reference_type find_min() const { return heap[1]; }

    void insert(const_reference_type x) {
        if (size_ == heap.size() - 1)
            heap.resize(heap.size() * 2);

        size_type hole = ++size_;
        for (; hole > 1 && x < heap[hole / 2]; hole /= 2)
            heap[hole] = heap[hole / 2];
        heap[hole] = x;
    }

    void erase_if(std::function<bool(const_reference_type)> f_) {
        if (empty())
            throw;
        int i = 1;
        for (; i < size_ + 1; i++) {
            if (f_(heap[i])) {
                break;
            }
        }
        if (i == size_ + 1) {
            return;
        }
        heap[i] = heap[size_--];
        percolate_down(i);
    }

    void delete_min() {
        if (empty())
            throw;

        heap[1] = heap[size_--];
        percolate_down(1);
    }

    void delete_min(reference_type x) {
        if (empty())
            throw;

        x = heap[1];
        heap[1] = heap[size_--];
        percolate_down(1);
    }

    void clear() {
        heap.clear();
        size_ = 0;
    }

private:
    size_type size_;
    std::vector<T> heap;

    void build() {
        for (int i = size_ / 2; i > 0; i--)
            percolate_down(i);
    }

    void percolate_down(size_type hole) {
        size_type child;
        T tmp = heap[hole];

        for (; hole * 2 <= size_; hole = child) {
            child = hole * 2;
            if (child != size_ && heap[child + 1] < heap[child])
                child++;
            if (heap[child] < tmp)
                heap[hole] = heap[child];
            else
                break;
        }
        heap[hole] = tmp;
    }
};
}

#endif // POLLERTEST_BINARYHEAP_H
