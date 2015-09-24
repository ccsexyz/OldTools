#include <iostream>
#include <cstring>
#include "Implement.h"
#include "Epoll.h"

#define DEFAULT_SERVER_NAME "echo"
#define DEFAULT_BIND_PORT   "9377"

int main() {
    int listenfd = Implement::initserver(DEFAULT_SERVER_NAME, DEFAULT_BIND_PORT);
    if(listenfd <= 0) {
        std::cerr << "init error!" << std::endl << "check the port please!" << std::endl;
        std::exit(1);
    }
    Implement::set_noblock(listenfd);

    Epoll *ep = new Epoll(0);
    Event *listen_event = new Event;
    listen_event->fd = listenfd;
    listen_event->event.data.ptr = listen_event;
    listen_event->event.events = EPOLLIN | EPOLLET;
    ep->add(listenfd, &(listen_event->event));

    auto handle_newcon = [&listenfd, &ep]()
        {
            while(1) {
                int clfd = accept(listenfd, NULL, NULL);
                if(clfd < 0) {
                    if(errno == EAGAIN) {
                        break;
                    } else {
                        std::cerr << "accept error : " << std::strerror(errno) << std::endl;
                        break;
                    }
                }
                Implement::set_noblock(clfd);
                Event *cl_event = new Event(clfd);
                ep->add(clfd, &(cl_event->event));
            }
        };

    auto event_handler = [&ep, &listenfd, &handle_newcon](struct epoll_event *events, int nfds)
        {
            if(nfds <= 0 || events == nullptr) return;
            for(int i = 0; i < nfds; i++) {
                struct epoll_event &event = events[i];
                Event *e = static_cast<Event *>(event.data.ptr);
                auto err_handler = [&ep](Event *p)
                    {
                        ep->del(p->fd, NULL);
                        delete p;
                    };
                auto in_handler = [&ep, &err_handler](Event *p)
                    {
                        auto ret = p->Read();
                        if(!ret && !p->flag) {
                            err_handler(p);
                            return false;
                        }
                        else {
                            p->event.events &= (~EPOLLIN);
                            p->event.events |= EPOLLOUT;
                            ep->mod(p->fd, &(p->event));
                            memcpy(p->wbuf, p->buf, p->offset);
                        }
                        return true;
                    };
                auto out_handler = [&ep, &err_handler](Event *p)
                    {
                        auto ret = p->Write();
                        if(!ret && !p->flag) {
                            err_handler(p);
                            return false;
                        }
                        else {
                            if(p->offset == 0) {
                                p->event.events &= (~EPOLLOUT);
                                p->event.events |= EPOLLIN;
                                ep->mod(p->fd, &(p->event));
                            }
                        }
                        return true;
                    };

                if(e->fd == listenfd) {
                    handle_newcon();
                    continue;
                } else if(event.events & EPOLLHUP || event.events & EPOLLERR) {
                    err_handler(e);
                    continue;
                } else if(event.events & EPOLLIN) {
                    if(!in_handler(e))
                        continue;
                } else if(event.events & EPOLLOUT) {
                    if(!out_handler(e))
                        continue;
                }
            }
        };

    ep->epoll_main_event_loop(event_handler);


    return 0;
}
