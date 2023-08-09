#include "thread_pool.hpp"

#include <stdexcept>
#include <utility>

#include "task.hpp"
#include "web_task.hpp"
namespace zy_http {
ThreadPool::ThreadPool(thread_n thread_count) : thread_count_(thread_count) {
    is_active_ = true;
    index_ = 0;

    ring_list_ = std::vector<IoUring>(thread_count);
    buffer_list_ = std::vector<BufferRing>(thread_count);

    for (unsigned i = 0; i < thread_count; ++i) {
        ring_list_[i].set_id(i);
    }

    for (unsigned i = 0; i < thread_count; ++i) {
        buffer_list_[i].register_buffer_ring(&ring_list_[i]);
    }

    for (unsigned i = 0; i < thread_count; ++i) {
        auto tmp = &ring_list_[i];
        thread_list_.emplace_back([this, tmp]() { run_loop(tmp); });
    }
}

ThreadPool::~ThreadPool() {
    is_active_.store(false, std::memory_order_seq_cst);
    for (auto& t : thread_list_) {
        t.join();
    }
}

auto ThreadPool::dispatch(int fd, IoUring* ring) -> void {
    struct io_uring_sqe* sqe = nullptr;
    IoUring* to_submit = nullptr;

    sqe = ring->get_sqe();

    if (sqe == nullptr) {
        throw std::runtime_error("failed to invoke 'main:get_sqe'");
    }
    sqe->user_data = 1;

    to_submit = &ring_list_[index_ % thread_count_];

    auto user_data_ptr = new user_data();

    user_data_ptr->only_coroutine = true;

    auto handle_client = static_web_server(fd, to_submit, &buffer_list_[index_ % thread_count_]);

    user_data_ptr->coroutine = handle_client.get_addr();

    handle_client.detach();

    io_uring_prep_msg_ring(sqe, to_submit->get_ring_fd(), 0,
                           reinterpret_cast<unsigned long long>(user_data_ptr), 0);

    ring->submit();

    index_++;
}

auto ThreadPool::run_loop(IoUring* local_ring) -> void {
    while (is_active_.load(std::memory_order_relaxed)) {
        auto cqe_ptr = local_ring->wait_cqe();

        if (cqe_ptr == nullptr) continue;
        user_data* user_data_ptr = reinterpret_cast<user_data*>(io_uring_cqe_get_data(cqe_ptr));

        if (user_data_ptr == nullptr) {
            local_ring->cqe_seen(cqe_ptr);
            continue;
        }

        user_data_ptr->cqe_res = cqe_ptr->res;
        user_data_ptr->cqe_flags = cqe_ptr->flags;
        auto coroutine_addr = user_data_ptr->coroutine;
        local_ring->cqe_seen(cqe_ptr);

        if (user_data_ptr->only_coroutine) {
            delete user_data_ptr;
        }

        if (coroutine_addr != nullptr) {
            std::coroutine_handle<>::from_address(coroutine_addr).resume();
        }
    }
}
}  // namespace zy_http