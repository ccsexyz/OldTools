#ifndef LIBY_UTIL_H
#define LIBY_UTIL_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <sys/time.h>

class DeferCaller final {
public:
    DeferCaller(std::function<void()> &&functor)
        : functor_(std::move(functor)) {}
    DeferCaller(const std::function<void()> &functor) : functor_(functor) {}
    ~DeferCaller() {
        if (functor_)
            functor_();
    }
    void cancel() { functor_ = nullptr; }
    DeferCaller(const DeferCaller &that) = delete;
    DeferCaller(DeferCaller &&that) = delete;
    DeferCaller &operator=(const DeferCaller &that) = delete;

private:
    std::function<void()> functor_;
};

// using gettimeofday
class Timestamp final {
public:
    static const uint32_t kMicrosecondsPerSecond = (1000) * (1000);

    Timestamp() : Timestamp(0, 0) {}

    explicit Timestamp(time_t sec) : Timestamp(sec, 0) {}

    explicit Timestamp(time_t sec, suseconds_t usec) {
        tv_.tv_sec = sec;
        tv_.tv_usec = usec;
        if (usec > kMicrosecondsPerSecond) {
            tv_.tv_sec += usec / kMicrosecondsPerSecond;
            tv_.tv_usec = usec % kMicrosecondsPerSecond;
        }
    }

    Timestamp(const Timestamp &that) { tv_ = that.tv_; }

    Timestamp(Timestamp &&that) { tv_ = that.tv_; }

    Timestamp &operator=(const Timestamp &that) {
        tv_ = that.tv_;
        return *this;
    }

    void swap(Timestamp &rhs) {
        auto temp(tv_);
        tv_ = rhs.tv_;
        rhs.tv_ = temp;
    }

    std::string toString() const {
        char buf[48] = {0};
        return std::string(ctime_r(&(tv_.tv_sec), buf));
    }

    bool invalid() const { return tv_.tv_sec == 0 && tv_.tv_usec == 0; }
    bool valid() const { return !invalid(); }

    time_t sec() const { return tv_.tv_sec; }
    suseconds_t usec() const { return tv_.tv_usec; }

    // if this > rhs return 1
    // elif == rhs return 0
    // elif < rhs return -1
    int compareTo(const Timestamp &rhs) const {
        if (rhs.tv_.tv_sec > tv_.tv_sec) {
            return -1;
        } else if (rhs.tv_.tv_sec < tv_.tv_sec) {
            return 1;
        } else {
            if (rhs.tv_.tv_usec > tv_.tv_usec) {
                return -1;
            } else if (rhs.tv_.tv_usec < tv_.tv_usec) {
                return 1;
            } else {
                return 0;
            }
        }
    }

    static Timestamp now() {
        Timestamp ret;
        ret.gettimeofday();
        return ret;
    }
    static Timestamp invalidTime() { return Timestamp(); }

private:
    void gettimeofday() { ::gettimeofday(&tv_, NULL); }

private:
    struct timeval tv_;
};

inline bool operator<(const Timestamp &lhs, const Timestamp &rhs) {
    return lhs.compareTo(rhs) == -1;
}

inline bool operator==(const Timestamp &lhs, const Timestamp &rhs) {
    return lhs.compareTo(rhs) == 0;
}

inline bool operator>(const Timestamp &lhs, const Timestamp &rhs) {
    return lhs.compareTo(rhs) == 1;
}

inline Timestamp operator-(const Timestamp &lhs, const Timestamp &rhs) {
    return Timestamp(lhs.sec() - rhs.sec(), lhs.usec() - rhs.usec());
}

inline Timestamp operator+(const Timestamp &lhs, const Timestamp &rhs) {
    return Timestamp(lhs.sec() + rhs.sec(), lhs.usec() + rhs.usec());
}

class Signal final {
public:
    static void signal(int signo, const std::function<void()> &handler);
    static void reset(int signo);
};

#endif // LIBY_UTIL_H
