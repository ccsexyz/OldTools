#include <iostream>
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

    auto handle_newcon = [&listenfd]()
        {
            while(1) {
                int clfd = accept(listenfd, NULL, NULL);
                if(clfd < 0) {
                    if(errno == EAGAIN) {
                        break;
                    } else {
                        std::cerr << "accept error : " << strerror(errno) << std::endl;
                        break;
                    }
                }
                Implement::set_noblock(clfd);
            }
        };

    Epoll *ep = new Epoll(0);
    Event *listen_event = new Event;
    listen_event->fd = listenfd;

    auto event_handler = [&ep, &listenfd](struct epoll_event *events, int nfds)
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
                auto in_handler = [&ep](Event *p)
                    {
                        auto ret = p->Read();
                        if(!ret && !p->flag)
                            err_handler(p);
                        else {
                            p->event &= (~EPOLLIN);
                            p->event |= EPOLLOUT;
                            ep->mod(p->fd, &p->event);
                        }
                    };
                auto out_handler = [&ep](Event *p)
                    {
                        auto ret = p->Write();
                        if(!ret && !p->flag)
                            err_handler(p);
                        else {
                            if(p->offset == 0) {
                                p->event &= (~EPOLLOUT);
                                p->event |= EPOLLIN;
                                ep->mod(p->fd, &p->event);
                            }
                        }
                    };

                if(e->fd == listenfd) {
                    handle_newcon();
                } else if(event.events & EPOLLHUP || event.events & EPOLLERR) {
                    err_handler(e);
                } else if(event.events & EPOLLIN) {
                    in_handler(e);
                } else if(event.events & EPOLLOUT) {
                    out_handler(e);
                }
            }
        };

    ep->epoll_main_event_loop(event_handler);


    return 0;
}