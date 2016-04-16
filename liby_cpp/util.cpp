#include "util.h"
#include "Poller.h"
#include <list>
#include <mutex>
#include <signal.h>
#include <unordered_map>

using namespace Liby;

static std::unordered_map<int, std::function<void()>> functors;
static std::unordered_map<int, void (*)(int)> stored_funcs;

static void signal_handler(int signo) { functors[signo](); }

void Signal::signal(int signo, const std::function<void()> &handler) {
    functors[signo] = handler;
    stored_funcs[signo] = ::signal(signo, signal_handler);
}

void Signal::reset(int signo) {
    auto k = functors.find(signo);
    if (k != functors.end()) {
        functors.erase(k);
    }
    auto k1 = stored_funcs.find(signo);
    if (k1 != stored_funcs.end()) {
        ::signal(signo, stored_funcs[signo]);
        stored_funcs.erase(k1);
    }
}

static std::mutex ExitCallerMutex;
static std::list<std::function<void()>> ExitCallerFunctors;

ExitCaller::ExitCaller() {
    std::cout << __func__ << std::endl;
    ::atexit(ExitCaller::callOnExit);
}

ExitCaller &ExitCaller::getCaller() {
    static ExitCaller caller;
    return caller;
}

void ExitCaller::call(const std::function<void()> &functor) {
    ExitCaller::getCaller().callImp(functor);
}

void ExitCaller::callImp(const std::function<void()> &functor) {
    std::lock_guard<std::mutex> G_(ExitCallerMutex);
    ExitCallerFunctors.push_back(functor);
}

void ExitCaller::callOnExit() {
    std::lock_guard<std::mutex> G_(ExitCallerMutex);
    std::cout << __func__ << std::endl;
    std::for_each(
        ExitCallerFunctors.begin(), ExitCallerFunctors.end(),
        [](decltype(*ExitCallerFunctors.begin()) &functor) { functor(); });
}

__thread time_t savedSec;
__thread char savedSecString[128]; // 似乎clang++并不支持thread_local的对象

std::string Timestamp::toString() const {
    std::string usecString = " " + std::to_string(tv_.tv_usec);
    if (tv_.tv_sec != savedSec) {
        struct tm result;
        ::localtime_r(&(tv_.tv_sec), &result);
        savedSec = tv_.tv_sec;
        sprintf(savedSecString, "%4d.%02d.%02d - %02d:%02d:%02d -",
                result.tm_year + 1900, result.tm_mon + 1, result.tm_mday,
                result.tm_hour, result.tm_min, result.tm_sec);
    }
    return savedSecString + usecString;
}

void TimerSet::cancelAllTimer() {
    assert(poller_);

    std::unordered_set<TimerId> timerIds; // timer handler may destroy this class
    timerIds.swap(timerIds_);
    for(auto &x : timerIds) {
        poller_->cancelTimer(x);
    }
}

void TimerSet::cancelTimer(TimerId id) {
    assert(poller_);
    timerIds_.erase(id);
    poller_->cancelTimer(id);
}

TimerId TimerSet::runAt(const Timestamp &timestamp, const BasicHandler &handler) {
    assert(poller_);
    TimerId id = poller_->runAt(timestamp, handler);
    timerIds_.insert(id);
    return id;
}

TimerId TimerSet::runAfter(const Timestamp &timestamp, const BasicHandler &handler) {
    assert(poller_);
    TimerId id = poller_->runAfter(timestamp, handler);
    timerIds_.insert(id);
    return id;
}

TimerId TimerSet::runEvery(const Timestamp &timestamp, const BasicHandler &handler) {
    assert(poller_);
    TimerId id = poller_->runEvery(timestamp, handler);
    timerIds_.insert(id);
    return id;
}