#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <boost/beast/http.hpp>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>
#include <string>
#include <string_view>
#include <map>
#include <optional>
#include <iostream>
#include <chrono>
#include <unordered_map>

#include "postgres.hpp"

namespace http = boost::beast::http;

namespace request { 
  struct CachedUserData {
    int user_id;
    std::string username;
    std::chrono::system_clock::time_point expiry;
  };

  extern size_t MAX_CACHE_SIZE;
  extern int CACHE_TTL_SECONDS;
  extern std::mutex cache_mutex;
  extern std::unordered_map<std::string, CachedUserData> session_cache;

  /* session nonsense */
  void invalidate_session(const std::string& session_id, int verbose);
  std::string get_session_id_from_cookie(const http::request<http::string_body>& req);

  std::map<std::string, std::string> parse_query_string(std::string_view query);
  std::optional<std::string> parse_from_request(const http::request<http::string_body>& req, const std::string& parameter);

  /* request responses */
  http::response<http::string_body> make_unauthorized_response(const std::string& message, const http::request<http::string_body>& req);
  http::response<http::string_body> make_bad_request_response(const std::string& message, const http::request<http::string_body>& req);
  http::response<http::string_body> make_ok_request_response(const std::string& message, const http::request<http::string_body>& req);
}
#endif