#include "File.h"
#include "Buffer.h"
#include "Logger.h"
#include <cstring>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#ifdef __linux__
#include <sys/eventfd.h>
#include <sys/sendfile.h>
#elif defined(__APPLE__)
#include <sys/uio.h>
#endif
#include "util.h"
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define QLEN (10)

using namespace Liby;

__thread char extraBuffer[163840];

static void set_noblock(int fd, bool flag = true) {
    int opt;
    opt = ::fcntl(fd, F_GETFL);
    if (opt < 0) {
        perror("fcntl get error!\n");
        exit(1);
    }
    opt = flag ? (opt | O_NONBLOCK) : (opt & ~O_NONBLOCK);
    if (::fcntl(fd, F_SETFL, opt) < 0) {
        perror("fcntl set error!\n");
        exit(1);
    }
}

void File::setNoblock(bool flag) { set_noblock(fd_, flag); }

void File::setNonagle(bool flag) {
    int nodelay = flag ? 1 : 0;
    if (::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int)) <
        0) {
        fprintf(stderr, "disable nagle error!\n");
    }
}

static int initsocket(int type, const struct sockaddr *addr, socklen_t alen,
                      int qlen) {
    int fd, err;
    int reuse = 1;

    if ((fd = ::socket(addr->sa_family, type, 0)) < 0)
        return (-1);
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
        goto errout;
    }
    if (::bind(fd, addr, alen) < 0) {
        goto errout;
    }
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
        if (::listen(fd, qlen) < 0) {
            goto errout;
        }
    return (fd);

errout:
    err = errno;
    close(fd);
    errno = err;
    return -1;
}

static int initserver(const char *server_path, const char *bind_port) {
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int sockfd, err;

    ::memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;
    if ((err = ::getaddrinfo(server_path, bind_port, &hint, &ailist)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
        return -1;
    }
    DeferCaller defer([ailist] { ::freeaddrinfo(ailist); });
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = initsocket(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen,
                                 QLEN)) >= 0) {
            set_noblock(sockfd);
            return sockfd;
        }
    }
    return -1;
}

FilePtr File::initserver(const std::string &server_path,
                         const std::string &server_port) {
    return std::make_shared<File>(
        ::initserver(server_path.data(), server_port.data()));
}

static int connect_tcp(const char *host, const char *port, int is_noblock) {
    int ret;
    struct addrinfo hints, *res;
    ::bzero((void *)(&hints), sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((ret = ::getaddrinfo(host, port, &hints, &res)) != 0) {
        fatal("getaddrinfo error: %s", ::gai_strerror(ret));
        return -1;
    }

    DeferCaller defer([res] { ::freeaddrinfo(res); });

    int sockfd;
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) ==
        -1)
        goto errout;

    if (is_noblock) {
        set_noblock(sockfd, true);
    }

    ret = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (ret < 0 && errno != EINPROGRESS) {
        error("connect error: %s\n", ::strerror(errno));
        ::close(sockfd);
        goto errout;
    }

    return sockfd;

errout:
    return -1;
}

FilePtr File::async_connect(const std::string &host, const std::string &port,
                            bool isNoblock) {
    return std::make_shared<File>(
        connect_tcp(host.data(), port.data(), isNoblock));
}

#ifdef OS_LINUX
FilePtr File::initeventfd() {
    int evfd = eventfd(10, EFD_CLOEXEC | EFD_NONBLOCK);
    return std::make_shared<File>(evfd);
}
#endif

ssize_t File::read(void *buf, size_t nbyte) {
    int ret = ::read(fd_, buf, nbyte);
    savedErrno_ = errno;
    return ret;
}

ssize_t File::write(void *buf, size_t nbyte) {
    int ret = ::write(fd_, buf, nbyte);
    savedErrno_ = errno;
    return ret;
}

void File::tryCloseFd() const {
    if (fd_ >= 0) {
        debug("try to close %d", fd_);
        ::close(fd_);
    } else {
        debug("try to close sth but fd is zero");
    }
}

ssize_t File::write(Buffer &buffer) {
    int ret = write(buffer.data(), buffer.size());
    if (ret > 0) {
        buffer.retrieve(ret);
    }
    return ret;
}

ssize_t File::read(Buffer &buffer) {
    struct iovec iov[2];
    iov[0].iov_base = buffer.wdata();
    iov[0].iov_len = buffer.availbleSize();
    iov[1].iov_base = extraBuffer;
    iov[1].iov_len = sizeof(extraBuffer);
    int ret = ::readv(fd_, iov, 2);
    if (ret > 0) {
        if (static_cast<decltype(iov[0].iov_len)>(ret) > iov[0].iov_len) {
            buffer.append(iov[0].iov_len);
            buffer.append(extraBuffer, ret - iov[0].iov_len);
        } else {
            buffer.append(ret);
        }
    }
    return ret;
}

std::vector<FilePtr> File::accept() {
    std::vector<FilePtr> ret;
    for (;;) {
        int clfd = ::accept(fd_, NULL, NULL);
        if (clfd < 0) {
            if (errno == EAGAIN) {
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                ret.push_back(std::make_shared<File>());
                return ret;
            }
        }
        ret.push_back(std::make_shared<File>(clfd));
    }
    return ret;
}

void File::shutdownRead() { ::shutdown(fd_, SHUT_RD); }

void File::shutdownWrit() { ::shutdown(fd_, SHUT_WR); }

int File::checkSocketError() { return 0; }

void File::sendFile(int fd, off_t size) {
#ifdef __linux__
    ::sendfile(fd_, fd, nullptr, size);
#elif defined(__APPLE__)
    ::sendfile(fd, fd_, 0, &size, nullptr, 0);
#endif
}

ssize_t File::write(std::deque<io_task> &tasks_) {
    ssize_t bytes = 0;
    while (!tasks_.empty()) {
        auto &first = tasks_.front();
        if (first.fp_) {
#ifdef __linux__
            off_t offset = first.offset_;
            ssize_t ret = ::sendfile(fd_, first.fp_->fd(), &offset, first.len_);
            if (ret < 0)
                return ret;
            if (ret == first.len_) {
                tasks_.pop_front();
                continue;
            } else {
                first.len_ -= ret;
                first.offset_ += ret;
                return -1;
            }
#elif defined(__APPLE__)
            off_t len = first.len_;
            int ret = ::sendfile(first.fp_->fd(), fd_, first.offset_, &len,
                                 nullptr, 0);
            if (ret < 0)
                return ret;
            // bytes += ret;
            if (len == first.len_) {
                tasks_.pop_front();
                continue;
            } else {
                first.len_ -= len;
                first.offset_ += len;
                return -1;
            }
#endif
        } else {
            std::vector<struct iovec> iov;
            for (auto &x : tasks_) {
                if (!x.fp_ && x.buffer_) {
                    struct iovec v;
                    v.iov_base = x.buffer_->data();
                    v.iov_len = x.buffer_->size();
                    iov.emplace_back(v);
                } else {
                    break;
                }
            }
            auto ret = ::writev(fd_, &iov[0], iov.size());
            if (ret > 0) {
                bytes += ret;
                while (ret > 0) {
                    auto firstBufferSize = tasks_.front().buffer_->size();
                    if (firstBufferSize <= ret) {
                        ret -= firstBufferSize;
                        tasks_.pop_front();
                    } else {
                        tasks_.front().buffer_->retrieve(ret);
                        return -1;
                    }
                }
            } else {
                return ret;
            }
        }
    }
    return bytes;
}
