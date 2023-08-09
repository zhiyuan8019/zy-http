#include "socket.hpp"

#include <netdb.h>

#include <cstring>
#include <iostream>
#include <string>

#include "constant.hpp"
#include "sys/socket.h"
namespace zy_http {

auto ServerSocket::bind(unsigned int port) -> void {
    addrinfo address_hints;
    addrinfo *socket_address;

    std::memset(&address_hints, 0, sizeof(addrinfo));
    address_hints.ai_family = AF_UNSPEC;
    address_hints.ai_socktype = SOCK_STREAM;
    address_hints.ai_flags = AI_PASSIVE;

    std::string port_string = std::to_string(port);

    if (getaddrinfo(nullptr, port_string.c_str(), &address_hints, &socket_address) != 0) {
        throw std::runtime_error("failed to invoke 'getaddrinfo'");
    }

    for (auto *node = socket_address; node != nullptr; node = node->ai_next) {
        raw_file_descriptor_ = socket(node->ai_family, node->ai_socktype, node->ai_protocol);
        if (raw_file_descriptor_.value() == -1) {
            throw std::runtime_error("failed to invoke 'socket'");
        }

        const int flag = 1;
        if (setsockopt(raw_file_descriptor_.value(), SOL_SOCKET, SO_REUSEADDR, &flag,
                       sizeof(flag)) == -1) {
            throw std::runtime_error("failed to invoke 'setsockopt'");
        }

        if (setsockopt(raw_file_descriptor_.value(), SOL_SOCKET, SO_REUSEPORT, &flag,
                       sizeof(flag)) == -1) {
            throw std::runtime_error("failed to invoke 'setsockopt'");
        }

        if (::bind(raw_file_descriptor_.value(), node->ai_addr, node->ai_addrlen) == -1) {
            throw std::runtime_error("failed to invoke 'bind'");
        }
        break;
    }
    freeaddrinfo(socket_address);
}

auto ServerSocket::listen() -> void {
    if (!raw_file_descriptor_.has_value()) {
        throw std::runtime_error("the file descriptor is invalid");
    }

    if (::listen(raw_file_descriptor_.value(), ZY_LISTEN_BACKLOG) == -1) {
        throw std::runtime_error("failed to invoke 'listen'");
    }
}

ClientSocket::ClientSocket(unsigned int fd, IoUring *local_ring, BufferRing *local_buffer)
    : FileDescriptor(fd), local_ring_(local_ring), local_buffer_(local_buffer) {}

auto ClientSocket::recv(void *buffer, size_t len) -> RecvAwaiter {
    return {raw_file_descriptor_.value(), buffer, len, local_ring_, local_buffer_};
}

auto ClientSocket::send(const std::span<char> &buffer, size_t len) -> SendAwaiter {
    return {raw_file_descriptor_.value(), buffer, len, local_ring_};
}

ClientSocket::RecvAwaiter::RecvAwaiter(int raw_file_descriptor, void *buffer, size_t length,
                                       IoUring *local_ring, BufferRing *local_buffer)
    : raw_file_descriptor_(raw_file_descriptor),
      buffer_(buffer),
      length_(length),
      local_ring_(local_ring),
      local_buffer_(local_buffer) {}

auto ClientSocket::RecvAwaiter::await_ready() const -> bool { return false; }

auto ClientSocket::RecvAwaiter::await_suspend(std::coroutine_handle<> coroutine) -> void {
    user_data_.coroutine = coroutine.address();
    local_ring_->prep_recv(&user_data_, raw_file_descriptor_, length_);
    // local_ring_->prep_recv_without_buffer(&user_data_, raw_file_descriptor_, buffer_, length_);
}

auto ClientSocket::RecvAwaiter::await_resume() -> std::tuple<unsigned int, ssize_t> {
    if (user_data_.cqe_flags | IORING_CQE_F_BUFFER) {
        const unsigned int buffer_id = user_data_.cqe_flags >> IORING_CQE_BUFFER_SHIFT;
        return {buffer_id, user_data_.cqe_res};
    }
    return std::tuple<unsigned int, ssize_t>(0, 0);
}

ClientSocket::SendAwaiter::SendAwaiter(int raw_file_descriptor, const std::span<char> &buffer,
                                       size_t length, IoUring *local_ring)
    : raw_file_descriptor_(raw_file_descriptor),
      length_(length),
      local_ring_(local_ring),
      buffer_(buffer) {}

auto ClientSocket::SendAwaiter::await_ready() const -> bool { return false; }

auto ClientSocket::SendAwaiter::await_suspend(std::coroutine_handle<> coroutine) -> void {
    user_data_.coroutine = coroutine.address();
    local_ring_->prep_send(&user_data_, raw_file_descriptor_, buffer_, length_);
}

auto ClientSocket::SendAwaiter::await_resume() -> ssize_t { return user_data_.cqe_res; }

}  // namespace zy_http