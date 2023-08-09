#include "file_descriptor.hpp"

#include <fcntl.h>

namespace zy_http {
FileDescriptor::FileDescriptor(const int raw_file_descriptor)
    : raw_file_descriptor_(raw_file_descriptor) {}

FileDescriptor::FileDescriptor(FileDescriptor &&other)
    : raw_file_descriptor_(other.raw_file_descriptor_) {
    other.raw_file_descriptor_ = std::nullopt;
}

FileDescriptor::~FileDescriptor() {
    if (raw_file_descriptor_.has_value()) {
        ::close(raw_file_descriptor_.value());
    }
}

auto FileDescriptor::get_raw_file_descriptor() const -> int { return raw_file_descriptor_.value(); }

auto open(const std::filesystem::path &path) -> FileDescriptor {
    const int raw_file_descriptor = ::open(path.c_str(), O_RDONLY);
    if (raw_file_descriptor == -1) {
        throw std::runtime_error("failed to invoke 'open'");
    }
    return FileDescriptor{raw_file_descriptor};
}

auto pipe() -> std::tuple<FileDescriptor, FileDescriptor> {
    std::array<int, 2> fd;
    if (::pipe(fd.data()) == -1) {
        throw std::runtime_error("failed to invoke 'pipe'");
    }
    return {FileDescriptor{fd[0]}, FileDescriptor{fd[1]}};
}

splice_awaiter::splice_awaiter(int raw_file_descriptor_in, int raw_file_descriptor_out,
                               size_t length, IoUring *local_ring)
    : raw_file_descriptor_in_(raw_file_descriptor_in),
      raw_file_descriptor_out_(raw_file_descriptor_out),
      length_(length),
      local_ring_(local_ring) {}

auto splice_awaiter::await_ready() const -> bool { return false; }
auto splice_awaiter::await_suspend(std::coroutine_handle<> coroutine) -> void {
    user_data_.coroutine = coroutine.address();
    local_ring_->prep_splice(&user_data_, raw_file_descriptor_in_, raw_file_descriptor_out_,
                             length_);
}
auto splice_awaiter::await_resume() const -> ssize_t { return user_data_.cqe_res; }
}  // namespace zy_http