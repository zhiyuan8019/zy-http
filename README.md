# `zy-http`

## `zy-http`: 基于`io_uring`和`C++ Coroutine`的静态资源HTTP服务器

与传统的基于Reactor+Epoll技术栈的服务器不同，`zy-http`利用了Linux下的最新的异步IO库 — `io_uring`，构建了一个高效的`Proactor`模式服务器。

**主要特性**：

- **简洁的异步调用**：使用C++ `Coroutine`的无栈协程对`io_uring`的异步IO接口进行了封装，使得异步调用变得简洁而直观。

- **高效的Proactor模式**：基于线程池和`io_uring`的ring_msg接口，实现了主线程的accept、内核IO以及工作线程进行逻辑处理，最大化减少IO阻塞和系统调用次数，确保了高效的任务处理和调度。

- **优化的生命周期与内存管理**：
  - 利用RAII技术进行资源的生命周期管理，确保资源的正确获取和释放。

  - 使用`unique_ptr`进行内存管理，减少内存泄漏的可能性。

- **零拷贝技术**：采用`splice`技术，零拷贝实现文件的读取和发送，避免了文件在用户和内核间进行拷贝。



## 软件架构

![](./image.png)

## 工作流程

- 主线程首先依据硬件并发数初始化对应数量的`io_uring`实例并为其注册环形缓冲区`ring_buffer`，之后完成server socket的初始化: `bind()`,`listen()`，并向主线程`io_uring`注册`multishot_accept`事件

- 主线程循环等待完成事件，从中获取`client`的文件描述符`fd`，并初始化协程任务（协程会在`initial_suspend()`调用后暂停）。之后主线程将协程地址作为`ring_msg`由主线程`io_uring`发送到工作线程的`io_uring`

- 工作线程循环等待线程所属的`io_uring`的完成事件，从中获取协程地址，恢复协程

- 协程依照处理逻辑完成套接字读取、http解析、数据发送，所有的套接字IO请求将会生成一个`io_uring`事件，并挂起当前协程，等待IO完成后由工作线程唤醒

## 协程伪代码

```cpp
auto coroutine(client_fd,io_uring*) -> Task<> {
    //客端初始化
    Client client(fd,io_ring*);
    //异步接收
    content = co_await client.recv();
    //报文解析
    HttpParser parser(content);
    if(valid_request){
        //200 OK
        HttpResponse response(200);
        //响应
        ret = co_await client.send(response);
        //内容
        ret = co_await client.send(file);
    }else{
        //404 Not Found
        HttpResponse response(404);
        //响应
        ret = co_await client.send(response);
    }
}

```

## 系统与编译

Linux：WSL Ubuntu22.04，内核版本6.4.8

liburing: 2.4

GCC: 13.1.0

C++标准: C++ 23

优化级别：O3

## 压测

压测软件使用`hey` [@GitHub](https://github.com/rakyll/hey)

测试环境： CPU : 6800H 8C16T，Memory : 16GB

总计100W请求，5000客户端并发，请求数据大小1K，所有请求23.3964秒完成


```console
./hey_linux_amd64 -n 1000000 -c 5000 http://127.0.0.1:80/1K

Summary:
  Total:        23.3964 secs
  Slowest:      1.5473 secs
  Fastest:      0.0001 secs
  Average:      0.1131 secs
  Requests/sec: 42741.6280
  
  Total data:   1024000000 bytes
  Size/request: 1024 bytes

Response time histogram:
  0.000 [1]     |
  0.155 [674752]        |■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
  0.310 [273064]        |■■■■■■■■■■■■■■■■
  0.464 [42504] |■■■
  0.619 [8219]  |
  0.774 [1281]  |
  0.928 [153]   |
  1.083 [21]    |
  1.238 [2]     |
  1.393 [2]     |
  1.547 [1]     |


Latency distribution:
  10% in 0.0005 secs
  25% in 0.0064 secs
  50% in 0.0887 secs
  75% in 0.1694 secs
  90% in 0.2511 secs
  95% in 0.3140 secs
  99% in 0.4597 secs

Details (average, fastest, slowest):
  DNS+dialup:   0.0032 secs, 0.0001 secs, 1.5473 secs
  DNS-lookup:   0.0000 secs, 0.0000 secs, 0.0000 secs
  req write:    0.0003 secs, 0.0000 secs, 0.2377 secs
  resp wait:    0.0021 secs, -0.0000 secs, 0.3284 secs
  resp read:    0.0452 secs, 0.0000 secs, 0.7930 secs

Status code distribution:
  [200] 1000000 responses

```

## 火焰图

perf采样了压测过程，瓶颈主要出现在close和io_uring_submit

![](./flamegraph.svg)