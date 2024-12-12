#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>
#include <string_view>
#include <map>
#include <optional>
#include <iostream>

namespace http = boost::beast::http;

namespace request {
  std::map<std::string, std::string> parse_query_string(std::string_view query);
  std::optional<std::string> parse_from_request(const http::request<http::string_body>& req, const std::string& parameter);

  http::response<http::string_body> make_bad_request_response(const std::string& message, const http::request<http::string_body>& req);
  http::response<http::string_body> make_ok_request_response(const std::string& message, const http::request<http::string_body>& req);
  http::response<http::string_body> handle_request(http::request<http::string_body> const& req);
}
#endif