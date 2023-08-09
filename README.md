# `zy-http`

`zy-http`是基于`io_uring`和`C++ Coroutine`编写的静态资源http服务器。与主流的基于Reactor+Epoll的服务器不同，本项目基于Linux下最新的异步IO库io_uring构建了Proactor模式的服务器。

- 基于C++ Coroutine无栈协程对io_uring的异步IO接口和服务过程进行包装，实现简洁的异步调用

- 基于线程池和io_uring的ring_msg接口实现主线程accept、内核IO、工作线程逻辑处理的Proactor模式

- 基于RAII的生命周期管理，基于uniqe_ptr的内存管理

- 基于splice的零拷贝发送


## 软件架构

![](./image.png)

## 系统与编译

Linux：WSL Ubuntu22.04，内核版本6.4.8

liburing: 2.4

GCC: 13.1.0

C++标准: C++ 23

优化级别：O3

## 压测

压测软件使用`hey` [@GitHub](https://github.com/rakyll/hey)

测试环境： CPU : 6800H 8C16T，Memory : 16GB

总计10W请求，5000客户端并发，请求数据大小1K，所有请求3.68秒完成


```console
./hey_linux_amd64 -n 100000 -c 5000 http://127.0.0.1:80/1K

Summary:
  Total:        3.6896 secs
  Slowest:      1.5335 secs
  Fastest:      0.0001 secs
  Average:      0.1674 secs
  Requests/sec: 27103.4861
  
  Total data:   102400000 bytes
  Size/request: 1024 bytes

Response time histogram:
  0.000 [1]     |
  0.153 [57073] |■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
  0.307 [29016] |■■■■■■■■■■■■■■■■■■■■
  0.460 [9027]  |■■■■■■
  0.613 [1686]  |■
  0.767 [1462]  |■
  0.920 [928]   |■
  1.073 [762]   |■
  1.227 [39]    |
  1.380 [5]     |
  1.533 [1]     |


Latency distribution:
  10% in 0.0012 secs
  25% in 0.0587 secs
  50% in 0.1171 secs
  75% in 0.2196 secs
  90% in 0.3367 secs
  95% in 0.4492 secs
  99% in 0.8977 secs

Details (average, fastest, slowest):
  DNS+dialup:   0.0072 secs, 0.0001 secs, 1.5335 secs
  DNS-lookup:   0.0000 secs, 0.0000 secs, 0.0000 secs
  req write:    0.0025 secs, 0.0000 secs, 0.6548 secs
  resp wait:    0.0194 secs, 0.0000 secs, 0.3821 secs
  resp read:    0.0494 secs, 0.0000 secs, 0.6313 secs

Status code distribution:
  [200] 100000 responses

```