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
