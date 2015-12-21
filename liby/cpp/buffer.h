#ifndef BUFFER_H
#define BUFFER_H

#include "base.h"

class Buffer;

using BUFFER_PTR = std::shared_ptr<Buffer>;

class Buffer : public std::enable_shared_from_this<Buffer>
{
public:
    using size_type = std::vector<char>::size_type;
    explicit Buffer(size_type buffersize0 = 4096)
    {
        buffer.reserve(buffersize0);
        buff = &buffer[0];
    }
    explicit Buffer(const char *buff0, size_type buffersize0) :
        size_(buffersize0)
    {
        buffer.reserve(buffersize0);
        buff = &buffer[0];
        memcpy(buff, buff0, buffersize0);
    }

    char *operator()(size_type i = 0) { return buff + i; }
    char *get_buff() const { return buff; }
    size_type capacity() const { return buffer.capacity(); }
    size_type size() const { return size_; }
    size_type off() const { return offset; }
    void add_off(size_type i) { offset += i; }
    void add_size(size_type i) { size_ += i; }
    void reset_off(size_type off0 = 0) { offset = off0; }
    void reset_size(size_type size0 = 0) { size_ = size0; }

    void clear()
    {
        buffer.clear();
        size_ = 0;
        offset = 0;
    }

    void reserve(std::vector<char>::size_type buffersize0)
    {
        buffer.reserve(buffersize0);
        buff = &buffer[0];
    }

    void append(char *src, size_type n)
    {
        if(size_ + n > capacity()) {
            reserve(size_ + n);
        }

        memcpy(buff + size_, src, n);
        add_size(n);
    }

    void append(const Buffer &buf)
    {
        append(buf.get_buff(), buf.off());
    }

private:
    char *buff;
    std::vector<char> buffer;
    size_type offset = 0;
    size_type size_ = 0;
};

#endif // BUFFER_H
