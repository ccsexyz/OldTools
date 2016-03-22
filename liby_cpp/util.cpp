#include "util.h"
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
