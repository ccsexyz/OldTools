##LibyCpp

###基本用法

```c++
#include "../Liby.h"

using namespace Liby;

int main() {
    EventLoop loop;
    auto echo_server = loop.creatTcpServer("localhost", "9377");
    echo_server->setReadEventCallback([](std::shared_ptr<Connection> &&conn) {
        conn->send(conn->read());
    });
    loop.RunMainLoop();
    return 0;
}
```

####设置工作线程数和IO服用借口
```c++
    EventLoop loop(8, EventLoop::PollerChooser::POLL);
```

####主动关闭连接
```c++
    echo_server->setReadEventCallback([](std::shared_ptr<Connection> &&conn) {
        conn->destroy();
    });
```

####设置IO循环终止条件
```c++
    // 默认为无限循环
    loop.RunMainLoop([]->bool{
        return false;
    });
```
###设置定时器
```c++
    // 指定绝对时刻
    conn->runAt(Timestamp(sec, usec), []{});
    // 指定相对时间
    conn->runAfter(Timestamp(sec, usec), []{});
    // 指定循环触发的定时器
    conn->runEvery(Timestamp(sec, usec), []{});
    // 取消定时器
    // 与conn绑定的定时器在conn析构或调用destroy时自动取消
    conn->cancelTimer(timerId);
```
