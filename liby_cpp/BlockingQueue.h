#ifndef LIBY_BLOCKINGQUEUE_H
#define LIBY_BLOCKINGQUEUE_H

#include <condition_variable>
#include <initializer_list>
#include <list>
#include <mutex>

template <typename T> class BlockingQueue {
public:
    using value_type = T;
    using reference_type = T &;
    using const_reference_type = const T &;

    BlockingQueue(std::initializer_list<T> elements = {}) {
        for (auto &e : elements) {
            queue_.emplace_back(e);
        }
    }

    void push_back(const_reference_type e) {
        std::lock_guard<std::mutex> G_(mutex_);
        queue_.emplace_back(e);
    }

    T pop_front() {
        std::lock_guard<std::mutex> G_(mutex_);
        auto t = queue_.front();
        queue_.pop_front();
        return t;
    }

    void pop() {
        std::lock_guard<std::mutex> G_(mutex_);
        queue_.pop_front();
    }

    T &front() { return queue_.front(); }

    bool empty() { return queue_.empty(); }

private:
    std::mutex mutex_;
    std::list<T> queue_;
};

#endif // LIBY_BLOCKINGQUEUE_H
