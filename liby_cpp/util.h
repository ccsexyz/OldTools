#ifndef LIBY_UTIL_H
#define LIBY_UTIL_H

#include "Logger.h"
#include <algorithm>
#include <assert.h>
#include <functional>
#include <iostream>
#include <memory>
#include <sys/time.h>
#include <type_traits>
#include <utility>

#if __cplusplus < 201402L

namespace std {
template <typename T, typename... Args>
inline typename enable_if<!is_array<T>::value, unique_ptr<T>>::type
make_unique(Args &&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
inline typename enable_if<is_array<T>::value && extent<T>::value == 0,
                          unique_ptr<T>>::type
make_unique(size_t size) {
    using U = typename remove_extent<T>::type;
    return unique_ptr<T>(new U[size]());
}

template <typename T, typename... Args>
typename enable_if<extent<T>::value != 0, void>::type
make_unique(Args &&...) = delete;
}

#endif

using BasicHandler = std::function<void()>;

struct clean_ {
public:
    clean_(const clean_ &) = delete;
    clean_(clean_ &&) = delete;
    clean_ &operator=(const clean_ &) = delete;
    clean_() = default;
    ~clean_() = default;
};

class DeferCaller final : clean_ {
public:
    DeferCaller(std::function<void()> &&functor)
        : functor_(std::move(functor)) {}
    DeferCaller(const std::function<void()> &functor) : functor_(functor) {}
    ~DeferCaller() {
        if (functor_)
            functor_();
    }
    void cancel() { functor_ = nullptr; }

private:
    std::function<void()> functor_;
};

class ErrnoSaver final : clean_ {
public:
    ErrnoSaver() : savedErrno_(errno) {}
    ~ErrnoSaver() { errno = savedErrno_; }

private:
    int savedErrno_ = 0;
};

class ExitCaller final : clean_ {
public:
    static ExitCaller &getCaller();
    static void call(const std::function<void()> &functor);

private:
    void callImp(const std::function<void()> &functor);
    static void callOnExit();

private:
    ExitCaller();
};

class DeconstructCaller : clean_ {
public:
    DeconstructCaller(std::initializer_list<BasicHandler> &&callerList = {}) {
        if (callerList.size() > 0) {
            handlers_ = new (std::nothrow) std::list<BasicHandler>;
            if (handlers_ == nullptr)
                return;
            for (auto &functor_ : callerList) {
                handlers_->emplace_back(functor_);
            }
        }
    }
    virtual ~DeconstructCaller() {
        if (handlers_ == NULL)
            return;
        for (auto &functor_ : *handlers_) {
            functor_();
        }
    }

private:
    std::list<BasicHandler> *handlers_ = nullptr;
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
        ctime_r(&(tv_.tv_sec), buf);
        for (int i = 0; buf[i]; i++) {
            if (buf[i] == '\r' || buf[i] == '\n')
                buf[i] = '\0';
        }
        return std::string(buf);
    }

    bool invalid() const { return tv_.tv_sec == 0 && tv_.tv_usec == 0; }
    bool valid() const { return !invalid(); }

    time_t sec() const { return tv_.tv_sec; }
    suseconds_t usec() const { return tv_.tv_usec; }

    uint64_t toMillSec() const {
        return tv_.tv_sec * 1000 + tv_.tv_usec / 1000;
    }
    double toSecF() const {
        return static_cast<double>(tv_.tv_usec) / 1000000.0 +
               static_cast<double>(tv_.tv_sec);
    }
    double toMillSecF() const {
        return static_cast<double>(tv_.tv_usec) / 1000.0 +
               static_cast<double>(tv_.tv_sec * 1000);
    }

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
    auto sec = lhs.sec() - rhs.sec();
    auto lusec = lhs.usec();
    auto rusec = rhs.usec();

    if (lusec < rusec) {
        sec--;
        lusec += Timestamp::kMicrosecondsPerSecond;
    }
    lusec -= rusec;

    return Timestamp(sec, lusec);
}

inline Timestamp operator+(const Timestamp &lhs, const Timestamp &rhs) {
    return Timestamp(lhs.sec() + rhs.sec(), lhs.usec() + rhs.usec());
}

class Signal final {
public:
    static void signal(int signo, const std::function<void()> &handler);
    static void reset(int signo);
};

class BaseException {
public:
    BaseException() = delete;
    BaseException(const std::string errmsg)
        : linenumber_(-1), errmsg_(errmsg) {}
    BaseException(const std::string &filename, int linenumber,
                  const std::string errmsg)
        : linenumber_(linenumber), filename_(filename), errmsg_(errmsg) {}
    virtual ~BaseException() = default;
    std::string what() const {
        if (linenumber_ >= 0)
            return filename_ + std::to_string(linenumber_) + errmsg_;
        else
            return errmsg_;
    }

private:
    int linenumber_;
    std::string filename_;
    std::string errmsg_;
};

template <typename T> void ClearUnuseVariableWarning(T &&) { /*do nothing*/
}

#endif // LIBY_UTIL_H
