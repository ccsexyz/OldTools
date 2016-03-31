#ifndef LIBY_File_H
#define LIBY_File_H

#include <memory>
#include <vector>

namespace Liby {
class File;
class Buffer;

using FilePtr = std::shared_ptr<File>;

class File final {
public:
    File(int fd = -1) : fd_(fd) {}
    ~File() { tryCloseFd(); }
    bool valid() const { return fd_ >= 0; }
    bool invalid() const { return fd_ < 0; }
    void resetfd() {
        tryCloseFd();
        fd_ = -1;
    }
    void setfd(int fd) {
        tryCloseFd();
        fd_ = fd;
    }
    int fd() const { return fd_; }
    void setNoblock(bool flag = true);
    void setNonagle(bool flag = true);
    void shutdownRead();
    void shutdownWrit();

    static FilePtr initserver(const std::string &server_path,
                              const std::string &server_port);
    static FilePtr async_connect(const std::string &host,
                                 const std::string &port,
                                 bool isNoblock = true);
    static FilePtr initeventfd();

    ssize_t read(void *buf, size_t nbyte);
    ssize_t write(void *buf, size_t nbyte);
    ssize_t read(Buffer &buffer);
    ssize_t write(Buffer &buffer);
    std::vector<FilePtr> accept();

    int getErrno() const { return savedErrno_; }

private:
    void tryCloseFd() const;

private:
    int fd_;
    int savedErrno_ = 0;
};
}

#endif // LIBY_File_H
