#include "connection.h"

Connection::Connection(SERVER_PTR server0, int fd0) :
    server(server0),
    event(std::make_shared<Event>(std::bind(&Connection::handle_event, this, std::placeholders::_1))),
    loop(server0->get_loop()),
    fd_(std::make_shared<fd_holder>(fd0)),
    fd(fd0)
{
    event->get_event_ptr()->events = EPOLLET;
    loop->add(fd_, event);
    events = &(event->get_event_ptr()->events);
}

void Connection::handle_event(EVENT_PTR)
{
    if(*events & EPOLLERR || *events & EPOLLHUP) {
        //error
        error();
        return;
    } else if(*events & EPOLLIN) {
        read_message();
    } else if(*events & EPOLLOUT) {
        write_message();
    }
}

void Connection::error()
{
    int err = errno;
    read_error(err);
    write_error(err);
}

void Connection::read_error(int err)
{
    if(current_read_task) {
        (*current_read_task)(err);
    }while(!read_queue.empty()) {
        auto x = read_queue.front();
        (*x)(err);
        read_queue.pop();
    }
}

void Connection::write_error(int err)
{
    if(current_write_task) {
        (*current_write_task)(err);
    }
    while(!write_queue.empty()) {
        auto x = write_queue.front();
        (*x)(err);
        write_queue.pop();
    }
}

void Connection::read_message()
{
    if(!current_read_task) {
        if(read_queue.empty()) {
            return;
        } else {
            current_read_task = read_queue.front();
            read_queue.pop();
        }
    }

    while(current_read_task) {
        auto buf = current_read_task->get_buffer();
        auto min = current_read_task->get_min_bytes();

        int ret = read(fd, (*buf)(buf->off()), buf->capacity() - buf->off());
        if(ret > 0) {
            buf->add_off(ret);
            if(buf->off() >= min) {
                buf->reset_size(buf->off());
                buf->reset_off();
                (*current_read_task)(0);
            } else {
                continue;
            }
        } else {
            int err = errno;
            if(ret != 0 && errno == EAGAIN) {
                if(read_queue.empty()) {
                    disable_read();
                }
            } else {
                read_error(err);
            }
            return;
        }

        if(read_queue.empty()) {
            current_read_task.reset();
            return;
        } else {
            current_read_task = read_queue.front();
            read_queue.pop();
        }
    }
}

void Connection::write_message()
{
    if(!current_write_task) {
        if(write_queue.empty()) {
            return;
        } else {
            current_write_task = write_queue.front();
            write_queue.pop();
        }
    }

    while(current_write_task) {
        auto buf = current_write_task->get_buffer();
        auto min = current_write_task->get_min_bytes();

        int ret = write(fd, (*buf)(buf->off()), buf->size() - buf->off());
        if(ret > 0) {
            buf->add_off(ret);
            if(buf->off() >= min) {
                (*current_write_task)(0);
            } else {
                continue;
            }
        } else {
            int err = errno;
            if(ret != 0 && errno == EAGAIN) {
                if(write_queue.empty()) {
                    disable_write();
                }
            } else {
                write_error(err);
            }
            return;
        }

        if(write_queue.empty()) {
            current_write_task.reset();
            return;
        } else {
            current_write_task = write_queue.front();
            write_queue.pop();
        }
    }
}

void Connection::read_queue_push(std::queue<IO_TASK_PTR>::value_type p)
{
    read_queue.push(p);
}

void Connection::write_queue_push(std::queue<IO_TASK_PTR>::value_type p)
{
    write_queue.push(p);
}

void Connection::async_read(BUFFER_PTR buf, io_handler handler)
{
    async_read_some(buf, 0, handler);
}

void Connection::async_read(io_handler handler)
{
    async_read_some(nullptr, 0, handler);
}

void Connection::async_write(BUFFER_PTR buf, io_handler handler)
{
    async_write_some(buf, buf->size(), handler);
}

void Connection::async_write(BUFFER_PTR buf)
{
    auto t = shared_from_this();
    async_write_some(buf, buf->off(), [t](BUFFER_PTR, int){});
}

void Connection::async_write(const char *buff, Buffer::size_type length, io_handler handler)
{
    async_write_some(std::make_shared<Buffer>(buff, length), length, handler);
}

void Connection::async_read_some(BUFFER_PTR buf, int min, io_handler handler)
{
    if(buf == nullptr) {
        buf = std::make_shared<Buffer>();
    }

    auto task = std::make_shared<io_task>(buf, min, handler);
    read_queue_push(task);
    enable_read();
}

void Connection::async_write_some(BUFFER_PTR buf, int min, io_handler handler)
{
    auto task = std::make_shared<io_task>(buf, min, handler);
    write_queue_push(task);
    enable_write();
}

void Connection::disable_read()
{
    *events &= ~(EPOLLIN);
    loop->mod(fd_, event);
}

void Connection::disable_write()
{
    *events &= ~(EPOLLOUT);
    loop->mod(fd_, event);
}

void Connection::enable_read()
{
    *events |= (EPOLLIN);
    loop->mod(fd_, event);
}

void Connection::enable_write()
{
    *events |= (EPOLLOUT);
    loop->mod(fd_, event);
}
