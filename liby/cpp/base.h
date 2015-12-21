#ifndef BASE_H
#define BASE_H

#include <iostream>
#include <memory>
#include <functional>
#include <algorithm>
#include <map>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <queue>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <linux/tcp.h>
#include <string.h>
#include <pthread.h>

class Mutex
{
public:
    Mutex()
    {
        if(pthread_mutex_init(&mutex0, NULL))
            std::exit(1);
    }
    ~Mutex()
    {
        pthread_mutex_destroy(&mutex0);
    }
    void lock()
    {
        pthread_mutex_lock(&mutex0);
    }
    void unlock()
    {
        pthread_mutex_unlock(&mutex0);
    }
    pthread_mutex_t *getMutexPtr()
    {
        return &mutex0;
    }

private:
    pthread_mutex_t mutex0;
};

class MutexGuard
{
public:
    MutexGuard(Mutex &mutex_)
            : mutex(mutex_)
    {
        mutex.lock();
    }
    ~MutexGuard()
    {
        mutex.unlock();
    }

private:
    Mutex &mutex;
};

class Condition
{
public:
    Condition(Mutex &mutex0) : mutex(mutex0)
    {
        if(pthread_cond_init(&cond, NULL))
            std::exit(1);
    }
    ~Condition()
    {
        pthread_cond_destroy(&cond);
    }
    void wait()
    {
        pthread_cond_wait(&cond, mutex.getMutexPtr());
    }
    void notify()
    {
        pthread_cond_signal(&cond);
        ;
    }
    void notifyAll()
    {
        pthread_cond_broadcast(&cond);
    }

private:
    pthread_cond_t cond;
    Mutex &mutex;
};

class fd_holder : public std::enable_shared_from_this<fd_holder>
{
public:
    explicit fd_holder(int fd0 = 0) : fd(fd0) {}
    ~fd_holder() { close(fd); }
    int get_fd() { return fd; }
    int operator()() { return fd; }
    void set_fd(int fd0) { fd = fd0; }
private:
    int fd;
};

using FD_HOLDER = std::shared_ptr<fd_holder>;

#endif // BASE_H
