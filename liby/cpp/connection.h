#ifndef CONNECTION_H
#define CONNECTION_H

#include "base.h"
#include "buffer.h"
#include "eventloop.h"
#include "server.h"
#include "io_task.h"

using CONNECTION_PTR = std::shared_ptr<Connection>;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    explicit Connection(SERVER_PTR server0, int fd0);
    void handle_event(EVENT_PTR);
    void read_message();
    void write_message();
    void error();
    void read_error(int err);
    void write_error(int err);
    void disable_read();
    void enable_read();
    void disable_write();
    void enable_write();
    void read_queue_push(std::queue<IO_TASK_PTR>::value_type p);
    void write_queue_push(std::queue<IO_TASK_PTR>::value_type p);
    void async_read(BUFFER_PTR buf, io_handler handler = nullptr);
    void async_read(io_handler handler);
    void async_read_some(BUFFER_PTR buf = nullptr, int min = 0, io_handler handler = nullptr);
    void async_write(BUFFER_PTR buf, io_handler handler);
    void async_write(BUFFER_PTR buf);
    void async_write_some(BUFFER_PTR buf, int min = 0, io_handler handler = nullptr);
    void async_write(const char *buff, Buffer::size_type length, io_handler handler = nullptr);
private:
    int fd;
    uint32_t *events;
    FD_HOLDER fd_;
    EVENT_PTR event;
    SERVER_PTR server;
    EVENTLOOP_PTR loop;

    IO_TASK_PTR current_read_task;
    IO_TASK_PTR current_write_task;
    std::queue<IO_TASK_PTR> read_queue;
    std::queue<IO_TASK_PTR> write_queue;
};

#endif // CONNECTION_H
