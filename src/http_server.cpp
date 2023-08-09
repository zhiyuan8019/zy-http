#include "http_server.hpp"

#include <netinet/in.h>

#include <cstring>

namespace zy_http {
HttpServer::HttpServer(unsigned int port, ThreadPool::thread_n thread_count)
    : thread_pool_{thread_count}, ring_{}, socket_{} {
    socket_.bind(port);
    socket_.listen();
}

auto HttpServer::run() -> void {
    int socketfd = socket_.get_raw_file_descriptor();
    ring_.prep_multishot_accept(socketfd);

    while (1) {
        io_uring_cqe *cqe = ring_.wait_cqe();
        auto fd = cqe->res;
        auto flag = cqe->flags;
        auto user_data = cqe->user_data;

        // ring msg cqe : just continue
        if (user_data == 1) {
            ring_.cqe_seen(cqe);
            continue;
        }
        // client socket: fd
        thread_pool_.dispatch(fd, &ring_);
        ring_.cqe_seen(cqe);
        if (!(flag & IORING_CQE_F_MORE)) {
            ring_.prep_multishot_accept(socketfd);
        }
    }
}
}  // namespace zy_http