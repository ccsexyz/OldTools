#ifndef LIBY_File_H
#define LIBY_File_H

#include "util.h"
#include <deque>
#include <memory>
#include <vector>

namespace Liby {
class FileDescriptor;
class Buffer;
struct io_task;

class FileDescriptor final {
public:
    FileDescriptor(int fd = -1) : fd_(fd) {
        if (fd < 0) {
            throw;
        }
    }
    ~FileDescriptor();
    int fd() const { return fd_; }

    void shutdownRead();
    void shutdownWrit();

private:
    int fd_;
};
}

#endif // LIBY_File_H
