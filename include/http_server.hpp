#ifndef ZY_HTTP_HTTP_SERVER_HPP
#define ZY_HTTP_HTTP_SERVER_HPP
#include <functional>

#include "io_uring.hpp"
#include "socket.hpp"
#include "task.hpp"
#include "thread_pool.hpp"
namespace zy_http {

class HttpServer {
  public:
    explicit HttpServer(unsigned int port = 80,
                        ThreadPool::thread_n thread_count = std::thread::hardware_concurrency());
    void run();

  private:
    ThreadPool thread_pool_;
    IoUring ring_;
    ServerSocket socket_;
};

}  // namespace zy_http

#endif  // ZY_HTTP_HTTP_SERVER_HPP