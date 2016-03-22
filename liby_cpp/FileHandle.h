#ifndef LIBY_FILEHANDLE_H
#define LIBY_FILEHANDLE_H

#include <memory>

class FileHandle;

using FileHandlePtr = std::shared_ptr<FileHandle>;

class FileHandle final {
public:
    FileHandle(int fd = -1) : fd_(fd) {}
    ~FileHandle() { tryCloseFd(); }
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

    static FileHandlePtr initserver(const std::string &server_path,
                                    const std::string &server_port);
    static FileHandlePtr async_connect(const std::string &host,
                                       const std::string &port,
                                       bool isNoblock = true);

    ssize_t read(void *buf, size_t nbyte);
    ssize_t write(void *buf, size_t nbyte);

    int getErrno() const { return savedErrno_; }

private:
    void tryCloseFd() const;

private:
    int fd_;
    int savedErrno_ = 0;
};

#endif // LIBY_FILEHANDLE_H
