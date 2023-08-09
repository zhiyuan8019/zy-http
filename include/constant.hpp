#ifndef ZY_HTTP_CONSTANT_HPP
#define ZY_HTTP_CONSTANT_HPP

namespace zy_http {
constexpr unsigned ZY_IO_URING_QUEUE_SIZE = 4096;

constexpr unsigned ZY_IO_URING_BUFFER_RING_SIZE = 8192;

constexpr unsigned ZY_IO_URING_BUFFER_SIZE = 1024;

constexpr unsigned ZY_LISTEN_BACKLOG = 511;

}  // namespace zy_http

#endif  // ZY_HTTP_CONSTANT_HPP