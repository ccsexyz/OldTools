#include "FileDescriptor.h"
#include <unistd.h>

using namespace Liby;

#ifdef OS_LINUX
FilePtr FileDescriptor::initeventfd() {
    int evfd = eventfd(10, EFD_CLOEXEC | EFD_NONBLOCK);
    return std::make_shared<FileDescriptor>(evfd);
}
#endif

FileDescriptor::~FileDescriptor() {
    if (fd_ >= 0) {
        verbose("try to close %d", fd_);
        ::close(fd_);
    } else {
        verbose("fd [%d] should be greater than zero", fd_);
    }
}

void FileDescriptor::shutdownRead() { ::shutdown(fd_, SHUT_RD); }

void FileDescriptor::shutdownWrit() { ::shutdown(fd_, SHUT_WR); }
