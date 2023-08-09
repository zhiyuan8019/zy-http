#ifndef ZY_HTTP_WEB_TASK_HPP
#define ZY_HTTP_WEB_TASK_HPP

#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <thread>

#include "constant.hpp"
#include "http_message.hpp"
#include "http_parser.hpp"
#include "io_uring.hpp"
#include "socket.hpp"
#include "task.hpp"
namespace zy_http {

Task<> static_web_server(unsigned int fd, IoUring *local_ring, BufferRing *local_buffer) {
    ClientSocket client_socket(fd, local_ring, local_buffer);

    HttpParser http_parser;
    // void *buffer = malloc(ZY_IO_URING_BUFFER_SIZE);
    void *buffer = nullptr;
    auto [recv_buffer_id, recv_buffer_size] =
        co_await client_socket.recv(buffer, ZY_IO_URING_BUFFER_SIZE);

    if (recv_buffer_size <= 0) {
        // free(buffer);
        co_return;
    }
    // std::span<char> recv_buffer(reinterpret_cast<char *>(buffer), recv_buffer_size);

    char *raw_recv_buffer = local_buffer->view_buffer(recv_buffer_id);
    std::span<char> recv_buffer(raw_recv_buffer, recv_buffer_size);

    if (const auto parse_result = http_parser.parse_http(recv_buffer); parse_result.has_value()) {
        const HttpRequest &http_request = parse_result.value();
        const std::filesystem::path file_path =
            std::filesystem::relative(http_request.request_path, "/");
        HttpResponse http_response;
        http_response.version = http_request.version;
        if (std::filesystem::exists(file_path) && std::filesystem::is_regular_file(file_path)) {
            http_response.code = "200";
            http_response.code_message = "OK";
            const uintmax_t file_size = std::filesystem::file_size(file_path);
            http_response.header_list.emplace_back("content-length", std::to_string(file_size));

            std::string send_content = http_response.serialize();
            std::span<char> send_buffer(send_content.data(), send_content.size());
            if (co_await client_socket.send(send_buffer, send_buffer.size()) == -1) {
                throw std::runtime_error("failed to invoke 'send'");
            }

            const FileDescriptor file_descriptor = open(file_path);

            const auto [read_pipe, write_pipe] = pipe();

            size_t bytes_sent = 0;
            while (bytes_sent < file_size) {
                {
                    ssize_t result = co_await splice_awaiter(
                        file_descriptor.get_raw_file_descriptor(),
                        write_pipe.get_raw_file_descriptor(), file_size, local_ring);
                    if (result < 0) {
                        // free(buffer);
                        local_buffer->return_buffer(local_ring, recv_buffer_id);
                        co_return;
                    }
                }
                {
                    ssize_t result = co_await splice_awaiter(
                        read_pipe.get_raw_file_descriptor(),
                        client_socket.get_raw_file_descriptor(), file_size, local_ring);
                    if (result < 0) {
                        // free(buffer);
                        local_buffer->return_buffer(local_ring, recv_buffer_id);
                        co_return;
                    }
                    bytes_sent += result;
                }
            }

        } else {
            http_response.code = "404";
            http_response.code_message = "Not Found";
            http_response.header_list.emplace_back("content-length", "0");

            std::string send_content = http_response.serialize();
            std::span<char> send_buffer(send_content.data(), send_content.size());

            if (co_await client_socket.send(send_buffer, send_buffer.size()) == -1) {
                throw std::runtime_error("failed to invoke 'send'");
            }
        }
    }
    // free(buffer);
    local_buffer->return_buffer(local_ring, recv_buffer_id);
}
}  // namespace zy_http
#endif  // ZY_HTTP_WEB_TASK_HPP