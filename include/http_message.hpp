#ifndef ZY_HTTP_HTTP_MESSAGE_HPP
#define ZY_HTTP_HTTP_MESSAGE_HPP

#include <string>
#include <tuple>
#include <vector>

namespace zy_http {
struct HttpRequest {
    std::string method;
    std::string request_path;
    std::string version;
    std::vector<std::tuple<std::string, std::string>> header_list;
};

struct HttpResponse {
    std::string version;
    std::string code;
    std::string code_message;
    std::vector<std::tuple<std::string, std::string>> header_list;

    auto serialize() const -> std::string;
};

}  // namespace zy_http

#endif  // ZY_HTTP_HTTP_MESSAGE_HPP