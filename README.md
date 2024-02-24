# couringserver
A web static resource server based on C++20 coroutine and asynchronous I/O framework io-uring.
## 项目介绍
### 概要

### 特性
* 使用 C++ 20 协程简化服务端与客户端的异步交互
* 使用 io_uring 管理异步 I/O 请求, 例如 accept(), recv(), send()
* 使用 ring-mapped buffers 减少内存分配的次数, 减少数据在内核态与用户态之间拷贝的次数 (Linux 5.19 新特性)
* 使用 multishot accept 减少向 io_uring 提交 accept() 请求的次数 (Linux 5.19 新特性)
* 实现线程池进行协程调度, 充分利用 CPU 的所有核心
* 使用 RAII 类管理 io_uring, 文件描述符, 以及线程池的生命周期
## 基本结构
## 编译环境
Linux Kernel 6.3 或更高版本
CMake 3.10 或更高版本
Clang 14 或更高版本
libstdc++ 11.3 或更高版本 (只要装 GCC 就可以)
liburing 2.3 或更高版本
## 构建
```
cmake -DCMAKE\_BUILD\_TYPE=Release -DCMAKE\_C\_COMPILER:FILEPATH=/usr/bin/clang -DCMAKE\_CXX\_COMPILER:FILEPATH=/usr/bin/clang++ -B build -G "Unix Makefiles"
make -C build -j$(nproc)
./build/couringserver
```
## 性能测试
为了测试 co-uring-http 在高并发情况的性能, 我用 hey 这个工具向它建立 1 万个客户端连接, 总共发送 100 万个 HTTP 请求, 每次请求大小为 1 KB 的文件. co-uring-http 每秒可以 88160 的请求, 并且在 0.5 秒内处理了 99% 的请求.

测试环境是 WSL (Ubuntu 22.04 LTS, Kernel 版本 6.3.0-microsoft-standard-WSL2), i5-12400 (6 核 12 线程 ), 16 GB 内存, PM9A1 固态硬盘.

```
./hey -n 1000000 -c 10000 <http://127.0.0.1:8080/1k>

Summary:
  Total:        11.3429 secs
  Slowest:      1.2630 secs
  Fastest:      0.0000 secs
  Average:      0.0976 secs
  Requests/sec: 88160.9738

  Total data:   1024000000 bytes
  Size/request: 1024 bytes

Response time histogram:
  0.000 [1]     |
  0.126 [701093]|■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
  0.253 [259407]|■■■■■■■■■■■■■■■
  0.379 [24843] |■
  0.505 [4652]  |
  0.632 [678]   |
  0.758 [1933]  |
  0.884 [1715]  |
  1.010 [489]   |
  1.137 [5111]  |
  1.263 [78]    |
```

## 设计文档
### 组件简介
task (task.hpp): task 类表示一个协程, 在被 co_await 之前不会启动.
thread_pool (thread_pool.hpp): thread_pool 类实现了一个线程池来调度协程.
file_descriptor (file_descriptor.hpp): file_descriptor 类持有一个文件描述符. file_descriptor.hpp 文件封装了一些支持 file_descriptor 类的系统调用，例如 open()、pipe() 与 splice().
server_socket (socket.hpp): server_socket 类扩展了 file_descriptor 类, 表示可接受客户端的监听套接字. 它提供了一个 accept() 方法, 记录是否在 io_uring 中存在现有的 multishot accept 请求，并在不存在时提交一个新的请求.
client_socket (socket.hpp): client_socket 类扩展了 file_descriptor 类, 表示与客户端进行通信的套接字. 它提供了一个 send() 方法, 用于向 io_uring 提交一个 send 请求, 以及一个 recv() 方法, 用于向 io_uring 提交一个 recv 请求.
io_uring (io_uring.hpp): io_uring 类是一个 thread_local 单例, 持有 io_urin 的提交队列与完成队列.
buffer_ring (buffer_ring.hpp): buffer_ring 类是一个 thread_local 单例, 向 io_uring 提供一组固定大小的缓冲区. 当收到一个 HTTP 请求时, io_uring 从 buffer_ring 中选择一个缓冲区用于存放收到的数据. 当这组数据被处理完毕后, buffer_ring 会将缓冲区还给 io_uring, 允许缓冲区被重复使用. 缓冲区的数量与大小的常量定义于 constant.hpp, 可以根据 HTTP 服务器的预估工作负载进行调整.
http_server (http_server.hpp): http_server 类为 thread_pool 中的每个线程创建一个 thread_worker 任务, 并等待这些任务执行完毕. (其实这些任务是个无限循环, 根本不会执行完毕 .)
thread_worker (http_server.hpp)：thread_worker 类提供了一些可以与客户端交互的协程. 它的构造函会启动 thread_worker::accept_client() 和 thread_worker::event_loop() 这两个协程.
thread_worker::event_loop() 协程在一个循环中处理 io_uring 的完成队列中的事件, 并继续运行等待该事件的协程.
thread_worker::accept_client() 协程在一个循环中通过调用 server_socket::accept() 来提交一个 multishot accept 请求到 io_uring. (由于 multishot accept 请求的持久性, server_socket::accept() 只有当之前的请求失效时才会提交新的请求到 io_uring.) 当新的客户端建立连接后, 它会启动 thread_worker::handle_client() 协程处理该客户端发来的 HTTP 请求.
thread_worker::handle_client() 协程调用 client_socket::recv() 来接收 HTTP 请求, 并且用 http_parser (http_parser.hpp) 解析 HTTP 请求. 等请求解析完毕后, 它会构造一个 http_response (http_message.hpp) 并调用 client_socket::send() 将响应发给客户端.
// `thread_worker::handle_client()` 协程的简化版代码逻辑
// 省略了许多用于处理 `http_request` 并构造 `http_response` 的代码
// 实现细节请参考源代码
auto thread_worker::handle_client(client_socket client_socket) -> task<> {
  http_parser http_parser;
  buffer_ring &buffer_ring = buffer_ring::get_instance();
  while (true) {
    const auto [recv_buffer_id, recv_buffer_size] = co_await client_socket.recv(BUFFER_SIZE);
    const std::span<char> recv_buffer = buffer_ring.borrow_buffer(recv_buffer_id, recv_buffer_size);
    // ...
    if (const auto parse_result = http_parser.parse_packet(recv_buffer); parse_result.has_value()) {
      const http_request &http_request = parse_result.value();
      // ...
      std::string send_buffer = http_response.serialize();
      co_await client_socket.send(send_buffer, send_buffer.size());
    }

    buffer_ring.return_buffer(recv_buffer_id);
  }
}

### 工作流程
http_server 为 thread_pool 中的每个线程创建一个 thread_worker 任务.
每个 thread_worker 任务使用 SO_REUSEPORT 选项创建一个套接字来监听相同的端口, 并启动 thread_worker::accept_client() 与 thread_worker::event_loop() 协程.
当新的客户端建立连接后, thread_worker::accept_client() 协程会启动 thread_worker::handle_client() 协程来处理该客户端的 HTTP 请求.
当 thread_worker::accept_client() 或 thread_worker::handle_client() 协程等待异步 I/O 请求时, 它会暂停执行并向 io_uring 的提交队列提交请求, 然后把控制权还给 thread_worker::event_loop().
thread_worker::event_loop() 处理 io_uring 的完成队列中的事件. 对于每个事件, 它会识别等待该事件的协程, 并恢复其执行.