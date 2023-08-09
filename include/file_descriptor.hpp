#ifndef ZY_HTTP_FILE_DESCRIPTOR_HPP
#define ZY_HTTP_FILE_DESCRIPTOR_HPP
#include <coroutine>
#include <filesystem>
#include <optional>

#include "io_uring.hpp"
#include "task.hpp"
#include "unistd.h"
namespace zy_http {
class FileDescriptor {
  public:
    FileDescriptor() = default;
    explicit FileDescriptor(int raw_file_descriptor);

    FileDescriptor(FileDescriptor &&other);

    FileDescriptor(const FileDescriptor &other) = delete;

    ~FileDescriptor();
    auto get_raw_file_descriptor() const -> int;

  protected:
    std::optional<int> raw_file_descriptor_;
};

class splice_awaiter {
  public:
    splice_awaiter(int raw_file_descriptor_in, int raw_file_descriptor_out, size_t length,
                   IoUring *local_ring);

    auto await_ready() const -> bool;
    auto await_suspend(std::coroutine_handle<> coroutine) -> void;
    auto await_resume() const -> ssize_t;

  private:
    const int raw_file_descriptor_in_;
    const int raw_file_descriptor_out_;
    const size_t length_;
    IoUring *const local_ring_;
    user_data user_data_;
};

auto open(const std::filesystem::path &path) -> FileDescriptor;

auto pipe() -> std::tuple<FileDescriptor, FileDescriptor>;

}  // namespace zy_http

#endif  // ZY_HTTP_FILE_DESCRIPTOR_HPP