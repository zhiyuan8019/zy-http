// #include <arpa/inet.h>
// #include <liburing.h>
// #include <sys/epoll.h>
// #include <sys/socket.h>
// #include <unistd.h>

// #include <algorithm>
// #include <bitset>
// #include <cstring>
// #include <iostream>
// #include <iterator>
// #include <map>
// #include <thread>
// #include <unordered_map>
// #include <vector>

// using namespace std;

// class TcpServer {
// public:
//     TcpServer() {
//         socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//     }
//     TcpServer(const char* ip, int port) {
//         socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//         bind_listen(ip, port);
//     }
//     ~TcpServer() {
//         release();
//     }
//     int bind_listen(const char* ip, int port) {
//         struct sockaddr_in serv_addr;
//         memset(&serv_addr, 0, sizeof(serv_addr));
//         serv_addr.sin_family = AF_INET;
//         serv_addr.sin_addr.s_addr = inet_addr(ip);
//         serv_addr.sin_port = htons(port);
//         bind(socket_, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
//         listen(socket_, 30);
//         return 0;
//     }

//     int wait_connect() {
//         struct sockaddr_in client_addr;
//         socklen_t client_addr_size = sizeof(client_addr);
//         return accept(socket_, (struct sockaddr*)&client_addr,
//                       &client_addr_size);
//     }

//     void release() {
//         if (socket_) {
//             close(socket_);
//             socket_ = 0;
//         }
//     }

// private:
//     int socket_;
// };

// int main() {
//     TcpServer server("127.0.0.1", 1234);
//     cout << "Start" << endl;
//     auto client = server.wait_connect();
//     vector<char> test_array(255);
//     // read(client, (char*)test_array.data(), sizeof(char) * test_array.size());
//     for (;;) {
//         auto recv_size = recv(client, (void*)test_array.data(), 255, 0);
//         for (int i = 0; i < recv_size; ++i) {
//             cout << test_array[i];
//         }
//         cout << endl;

//         auto send_size = send(client, (void*)test_array.data(), 255, 0);
//     }
//     return 0;
// }

#include "http_server.hpp"

using namespace zy_http;

int main() {
    HttpServer server;
    server.run();
    return 0;
}