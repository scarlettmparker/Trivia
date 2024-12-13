#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <boost/beast/http.hpp>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>
#include <string_view>
#include <map>
#include <optional>
#include <iostream>

#include "postgres.hpp"

namespace http = boost::beast::http;

namespace request {
  std::map<std::string, std::string> parse_query_string(std::string_view query);
  std::optional<std::string> parse_from_request(const http::request<http::string_body>& req, const std::string& parameter);

  http::response<http::string_body> make_unauthorized_response(const std::string& message, const http::request<http::string_body>& req);
  http::response<http::string_body> make_bad_request_response(const std::string& message, const http::request<http::string_body>& req);
  http::response<http::string_body> make_ok_request_response(const std::string& message, const http::request<http::string_body>& req);
  
  std::string get_session_id_from_cookie(const http::request<http::string_body>& req);
  int select_user_id_from_session(const std::string& session_id, int verbose=0);
  http::response<http::string_body> validate_session(const http::request<http::string_body>& req);
}
#endif