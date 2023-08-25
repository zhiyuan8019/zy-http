#include "io_uring.hpp"

#include <stdexcept>

#include "constant.hpp"
namespace zy_http {
IoUring::IoUring() {
    if (auto result = io_uring_queue_init(ZY_IO_URING_QUEUE_SIZE, &ring_, 0); result != 0) {
        throw ::std::runtime_error("Failed : io_uring_queue_init");
    }
};

IoUring::~IoUring() { io_uring_queue_exit(&ring_); }

auto IoUring::begin() -> CqeIterator { return CqeIterator{&ring_, *ring_.cq.khead}; }

auto IoUring::end() -> CqeIterator {
    return CqeIterator{&ring_, io_uring_smp_load_acquire(ring_.cq.ktail)};
}

auto IoUring::cqe_seen(io_uring_cqe *cqe) -> void { io_uring_cqe_seen(&ring_, cqe); }

auto IoUring::setup_buffer_ring(io_uring_buf_ring *buffer_ring,
                                std::vector<std::unique_ptr<char[]>> *buffer_list,
                                const unsigned int buffer_size, const unsigned int buffer_ring_size)
    -> void {
    io_uring_buf_reg reg = {};
    reg.ring_addr = reinterpret_cast<unsigned long>(buffer_ring);
    reg.ring_entries = buffer_ring_size;
    reg.bgid = id_;

    const int result = io_uring_register_buf_ring(&ring_, &reg, 0);
    if (result != 0) {
        throw std::runtime_error("failed to invoke 'io_uring_register_buf_ring'");
    }
    io_uring_buf_ring_init(buffer_ring);
    for (unsigned i = 0; i < buffer_ring_size; i++) {
        io_uring_buf_ring_add(buffer_ring, reinterpret_cast<void *>(buffer_list->at(i).get()),
                              buffer_size, i, io_uring_buf_ring_mask(buffer_ring_size), i);
    }
    io_uring_buf_ring_advance(buffer_ring, buffer_ring_size);
}

auto IoUring::add_buffer_ring(io_uring_buf_ring *buffer_ring, char *buffer, unsigned int buffer_id,
                              unsigned int buffer_ring_size) -> void {
    io_uring_buf_ring_add(buffer_ring, reinterpret_cast<void *>(buffer), ZY_IO_URING_BUFFER_SIZE,
                          buffer_id, io_uring_buf_ring_mask(buffer_ring_size), 0);
    /* make it visible */
    io_uring_buf_ring_advance(buffer_ring, 1);
}

auto IoUring::prep_multishot_accept(int sockfd) -> void {
    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    if (sqe == nullptr) {
        throw std::runtime_error("failed to invoke 'io_uring_get_sqe'");
    }
    io_uring_prep_multishot_accept(sqe, sockfd, nullptr, nullptr, 0);
    io_uring_submit_and_wait(&ring_, 1);
}

auto IoUring::prep_splice(user_data *sqe_data, int raw_file_descriptor_in,
                          int raw_file_descriptor_out, size_t length) -> void {
    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    sqe->opcode = IORING_OP_SPLICE;

    io_uring_prep_splice(sqe, raw_file_descriptor_in, -1, raw_file_descriptor_out, -1, length, 0);

    io_uring_sqe_set_data(sqe, sqe_data);
    int ret = io_uring_submit(&ring_);
    if (ret < 0) {
        throw std::runtime_error("failed to invoke 'prep_splice:io_uring_submit'");
    }
}

auto IoUring::prep_recv(user_data *sqe_data, const int raw_file_descriptor, const size_t length)
    -> void {
    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_recv(sqe, raw_file_descriptor, nullptr, length, 0);
    sqe->buf_group = id_;
    io_uring_sqe_set_flags(sqe, IOSQE_BUFFER_SELECT);
    io_uring_sqe_set_data(sqe, sqe_data);
    io_uring_submit(&ring_);
}

auto IoUring::prep_recv_without_buffer(user_data *sqe_data, const int raw_file_descriptor,
                                       void *buffer, const size_t length) -> void {
    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_recv(sqe, raw_file_descriptor, buffer, length, 0);
    io_uring_sqe_set_data(sqe, sqe_data);
    io_uring_submit(&ring_);
}

auto IoUring::prep_send(user_data *sqe_data, const int raw_file_descriptor,
                        const std::span<char> &buffer, const size_t length) -> void {
    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_send(sqe, raw_file_descriptor, buffer.data(), length, 0);
    io_uring_sqe_set_data(sqe, sqe_data);
    io_uring_submit(&ring_);
}

auto IoUring::submit_and_wait(unsigned wait_nr) -> void {
    io_uring_submit_and_wait(&ring_, wait_nr);
}
auto IoUring::wait_cqe() -> io_uring_cqe * {
    io_uring_cqe *cqe;
    io_uring_wait_cqe(&ring_, &cqe);
    return cqe;
}
auto IoUring::get_sqe() -> io_uring_sqe * { return io_uring_get_sqe(&ring_); }

auto IoUring::get_ring_fd() -> int { return ring_.ring_fd; }

auto IoUring::submit() -> void { io_uring_submit(&ring_); }

auto IoUring::set_id(unsigned id) -> void { id_ = id; }

auto IoUring::get_id() -> unsigned { return id_; }

IoUring::CqeIterator::CqeIterator(const ::io_uring *uring, const unsigned int head)
    : uring_(uring), head_(head) {}

auto IoUring::CqeIterator::operator++() -> CqeIterator & {
    ++head_;
    return *this;
}

auto IoUring::CqeIterator::operator*() const -> io_uring_cqe * {
    return &uring_->cq.cqes[io_uring_cqe_index(uring_, head_, (uring_)->cq.ring_mask)];
}

auto IoUring::CqeIterator::operator!=(const CqeIterator &right) const -> bool {
    return head_ != right.head_;
}

auto BufferRing::register_buffer_ring(IoUring *to_register) -> void {
    const size_t ring_entries_size = ZY_IO_URING_BUFFER_RING_SIZE * sizeof(io_uring_buf_ring);
    const size_t page_alignment = sysconf(_SC_PAGESIZE);
    void *buffer_ring = nullptr;
    posix_memalign((void **)&buffer_ring, page_alignment, ring_entries_size);

    buffer_ring_.reset(reinterpret_cast<io_uring_buf_ring *>(buffer_ring));
    buffer_list_ = std::vector<std::unique_ptr<char[]>>(ZY_IO_URING_BUFFER_RING_SIZE);

    for (unsigned int i = 0; i < ZY_IO_URING_BUFFER_RING_SIZE; ++i) {
        // void *tmp = nullptr;
        // posix_memalign((void **)&tmp, page_alignment, ZY_IO_URING_BUFFER_SIZE * sizeof(char));
        char *tmp = new char[ZY_IO_URING_BUFFER_SIZE];
        buffer_list_[i].reset(reinterpret_cast<char *>(tmp));
    }

    to_register->setup_buffer_ring(buffer_ring_.get(), &buffer_list_, ZY_IO_URING_BUFFER_SIZE,
                                   ZY_IO_URING_BUFFER_RING_SIZE);
}

auto BufferRing::view_buffer(const unsigned int buffer_id) -> char * {
    return buffer_list_[buffer_id].get();
}

auto BufferRing::return_buffer(IoUring *to_return, const unsigned int buffer_id) -> void {
    to_return->add_buffer_ring(buffer_ring_.get(), buffer_list_[buffer_id].get(), buffer_id,
                               buffer_list_.size());
}

auto BufferRing::get_buf_size() -> unsigned { return buf_size_; }

}  // namespace zy_http