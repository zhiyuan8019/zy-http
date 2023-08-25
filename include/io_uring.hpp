#ifndef ZY_HTTP_IO_URING_HPP
#define ZY_HTTP_IO_URING_HPP
#include <liburing.h>

#include <memory>
#include <span>
#include <vector>

#include "constant.hpp"
namespace zy_http {

struct user_data {
    void *coroutine = nullptr;
    int cqe_res = 0;
    unsigned int cqe_flags = 0;
    bool only_coroutine = false;
};

struct FreeDeleter {
    void operator()(void *ptr) const { free(ptr); }
};

class IoUring {
  private:
    ::io_uring ring_;
    unsigned id_;

  public:
    IoUring();
    ~IoUring();

    class CqeIterator {
      public:
        explicit CqeIterator(const ::io_uring *uring, const unsigned int head);
        auto operator++() -> CqeIterator &;
        auto operator*() const -> io_uring_cqe *;
        auto operator!=(const CqeIterator &right) const -> bool;

      private:
        const ::io_uring *uring_;
        unsigned int head_;
    };

    auto begin() -> CqeIterator;

    auto end() -> CqeIterator;

    auto cqe_seen(io_uring_cqe *cqe) -> void;

    auto setup_buffer_ring(io_uring_buf_ring *buffer_ring,
                           std::vector<std::unique_ptr<char[]>> *buffer_list,
                           const unsigned int buffer_size, const unsigned int buffer_ring_size)
        -> void;

    auto add_buffer_ring(io_uring_buf_ring *buffer_ring, char *buffer, unsigned int buffer_id,
                         unsigned int buffer_ring_size) -> void;

    auto prep_multishot_accept(int sockfd) -> void;

    auto prep_splice(user_data *sqe_data, int raw_file_descriptor_in, int raw_file_descriptor_out,
                     size_t length) -> void;
    auto prep_recv(user_data *sqe_data, const int raw_file_descriptor, const size_t length) -> void;

    auto prep_recv_without_buffer(user_data *sqe_data, const int raw_file_descriptor, void *buffer,
                                  const size_t length) -> void;

    auto prep_send(user_data *sqe_data, const int raw_file_descriptor,
                   const std::span<char> &buffer, const size_t length) -> void;

    auto submit_and_wait(unsigned wait_nr) -> void;

    auto wait_cqe() -> io_uring_cqe *;

    auto get_sqe() -> io_uring_sqe *;

    auto get_ring_fd() -> int;

    auto submit() -> void;

    auto set_id(unsigned) -> void;

    auto get_id() -> unsigned;
};

// no use : has bug when multithread

class BufferRing {
  public:
    BufferRing() = default;
    auto register_buffer_ring(IoUring *to_register) -> void;
    auto view_buffer(const unsigned int buffer_id) -> char *;
    auto return_buffer(IoUring *to_return, const unsigned int buffer_id) -> void;
    auto get_buf_size() -> unsigned;

  private:
    unsigned buf_size_ = ZY_IO_URING_BUFFER_SIZE;
    std::unique_ptr<io_uring_buf_ring, FreeDeleter> buffer_ring_;
    std::vector<std::unique_ptr<char[]>> buffer_list_;
};

}  // namespace zy_http

#endif  // ZY_HTTP_IO_URING_HPP