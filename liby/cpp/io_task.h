#ifndef IO_TASK_H
#define IO_TASK_H

#include "base.h"
#include "buffer.h"

class io_task;

using io_handler = std::function<void(BUFFER_PTR, int)>;
using IO_TASK_PTR = std::shared_ptr<io_task>;

class io_task : public std::enable_shared_from_this<io_task>
{
public:
    explicit io_task(BUFFER_PTR &buf0, Buffer::size_type min_except_bytes0, io_handler handler0) :
        buf(buf0),
        min_except_bytes(min_except_bytes0),
        handler(handler0)
    {}
    void operator()(int ec = 0)
    {
        if(handler) {
            handler(buf, ec);
        }
    }
    Buffer::size_type get_min_bytes() { return min_except_bytes; }
    BUFFER_PTR get_buffer() { return buf; }

private:
    BUFFER_PTR buf;
    Buffer::size_type min_except_bytes;
    io_handler handler;
};

#endif // IO_TASK_H
