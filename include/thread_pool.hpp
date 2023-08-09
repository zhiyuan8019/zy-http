#ifndef ZY_HTTP_THREAD_POOL_HPP
#define ZY_HTTP_THREAD_POOL_HPP

#include <atomic>
#include <thread>
#include <vector>

#include "file_descriptor.hpp"
#include "io_uring.hpp"
#include "task.hpp"
namespace zy_http {

class ThreadPool {
  public:
    using thread_n = std::invoke_result<decltype(std::thread::hardware_concurrency)>::type;

    explicit ThreadPool(thread_n thread_count);

    ~ThreadPool();

    auto dispatch(int fd, IoUring *ring) -> void;

  private:
    auto run_loop(IoUring *) -> void;
    unsigned long index_;
    std::atomic<bool> is_active_;
    thread_n thread_count_;
    std::vector<IoUring> ring_list_;
    std::vector<BufferRing> buffer_list_;
    std::vector<std::jthread> thread_list_;
    std::vector<FileDescriptor> file_list_;
};

}  // namespace zy_http

#endif  // ZY_HTTP_THREAD_POOL_HPP