#ifndef ZY_HTTP_HTTP_PARSER_HPP
#define ZY_HTTP_HTTP_PARSER_HPP

#include <optional>
#include <span>
#include <string>

#include "http_message.hpp"
namespace zy_http {
struct http_request;

class HttpParser {
  public:
    auto parse_http(std::span<char> packet) -> std::optional<HttpRequest>;

  private:
    std::string raw_http_request_;
};

}  // namespace zy_http

#endif  // ZY_HTTP_HTTP_PARSER_HPP
