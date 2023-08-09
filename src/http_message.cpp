#include "http_message.hpp"

#include <sstream>

namespace zy_http {

auto HttpResponse::serialize() const -> std::string {
    std::stringstream raw_http_response;
    raw_http_response << version << ' ' << code << ' ' << code_message << "\r\n";
    for (const auto &[k, v] : header_list) {
        raw_http_response << k << ':' << v << "\r\n";
    }
    raw_http_response << "\r\n";
    return raw_http_response.str();
}

}  // namespace zy_http