#ifndef ZY_HTTP_SOCKET_HPP
#define ZY_HTTP_SOCKET_HPP

#include "file_descriptor.hpp"
namespace zy_http {

class ServerSocket : public FileDescriptor {
  public:
    ServerSocket() = default;
    auto bind(unsigned int port) -> void;
    auto listen() -> void;
};

class ClientSocket : public FileDescriptor {
  public:
    ClientSocket(unsigned int fd, IoUring *local_ring, BufferRing *local_buffer);
    class RecvAwaiter {
      public:
        RecvAwaiter(int raw_file_descriptor, void *buffer, size_t length, IoUring *local_ring,
                    BufferRing *local_buffer);
        auto await_ready() const -> bool;
        auto await_suspend(std::coroutine_handle<> coroutine) -> void;
        auto await_resume() -> std::tuple<unsigned int, ssize_t>;

      private:
        user_data user_data_;
        const int raw_file_descriptor_;
        void *buffer_;
        const size_t length_;
        IoUring *const local_ring_;
        BufferRing *const local_buffer_;
    };

    class SendAwaiter {
      public:
        SendAwaiter(int raw_file_descriptor, const std::span<char> &buffer, size_t length,
                    IoUring *local_ring);
        auto await_ready() const -> bool;
        auto await_suspend(std::coroutine_handle<> coroutine) -> void;
        auto await_resume() -> ssize_t;

      private:
        const int raw_file_descriptor_;
        const size_t length_;
        user_data user_data_;
        IoUring *const local_ring_;
        const std::span<char> &buffer_;
    };

    auto recv(void *buffer, size_t len) -> RecvAwaiter;
    auto send(const std::span<char> &buffer, size_t len) -> SendAwaiter;

  private:
    IoUring *local_ring_;
    BufferRing *local_buffer_;
};

}  // namespace zy_http

#endif  // ZY_HTTP_SOCKET_HPP