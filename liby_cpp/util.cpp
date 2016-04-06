#include "util.h"
#include <list>
#include <mutex>
#include <signal.h>
#include <unordered_map>

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
