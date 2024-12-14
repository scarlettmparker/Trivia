#include "request.hpp"

using namespace postgres;
namespace request {
  /* caching for session data */
  size_t MAX_CACHE_SIZE = 1000;
  int CACHE_TTL_SECONDS = 60;
  std::mutex cache_mutex;
  std::unordered_map<std::string, CachedUserData> session_cache;

  /**
   * Invalidate a session by setting it to inactive.
   * @param session_id Session ID to invalidate.
   * @param verbose Whether to print messages to stdout.
   */
  void invalidate_session(const std::string& session_id, int verbose) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    session_cache.erase(session_id);
    try {
      pqxx::work txn(*postgres::c);
      txn.exec_prepared("invalidate_session", session_id);
      txn.commit();
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
  }

  /**
   * Get the session ID from a cookie in a request.
   * @param req Request to get the session ID from.
   * @return Session ID from the cookie.
   */
  std::string get_session_id_from_cookie(const http::request<http::string_body>& req) {
    auto cookie_header = req[http::field::cookie];
    std::string session_id;

    size_t pos = cookie_header.find("sessionId=");
    if (pos != std::string::npos) {
      session_id = std::string(cookie_header.substr(pos + 10));
    }
    return session_id;
  }

   /**
   * Parse a query string into a map of key-value pairs
   * This allows query strings to be extracted from URLS, to be used as parameters.
   *
   * @param query Query string to parse.
   * @return Map of key-value pairs from the query string.
   */
  std::map<std::string, std::string> parse_query_string(std::string_view query) {
    std::map<std::string, std::string> params;
    size_t start = 0;

    while (start < query.length()) {
      size_t end = query.find('&', start);
      if (end == std::string::npos) {
        end = query.length();
      }

      size_t equals = query.find('=', start);
      if (equals != std::string::npos && equals < end) {
        std::string key = std::string(query.substr(start, equals - start));
        std::string value = std::string(query.substr(equals + 1, end - equals - 1));
        params[key] = value;
      }

      start = end + 1;
    }
    return params;
  }

  /**
   * Parse the given parameter from a request. This ensures that the parameter
   * parameter appears in the query string. If it does not, an empty optional is returned.
   *
   * @param req Request to parse the parameter from.
   * @param parameter Parameter to parse from the request.
   * @return Value of the parameter if it exists, empty optional otherwise.
   */
  std::optional<std::string> parse_from_request(const http::request<http::string_body>& req, const std::string& parameter) {
    std::string target_str(req.target());
    std::string_view target(target_str);
    auto query_pos = target.find('?');

    if (query_pos == std::string::npos) {
        std::cout << "No query string found in URL" << std::endl;
        return std::nullopt;
    }

    auto query = target.substr(query_pos + 1);
    auto params = parse_query_string(std::string(query));

    if (params.find(parameter) == params.end()) {
        std::cout << "Missing category parameter" << std::endl;
        return std::nullopt;
    }
    
    return params[parameter];
  }

  /**
   * Create an unauthorized response with a given message.
   * @param message Message to include in the response.
   * @param req Request that caused the error.
   * @return Response with the given message.
   */
  http::response<http::string_body> make_unauthorized_response(
    const std::string& message, const http::request<http::string_body>& req) {
    
    http::response<http::string_body> res{http::status::unauthorized, req.version()};
    res.set(http::field::server, "Beast");
    res.set(http::field::content_type, "application/json");

    nlohmann::json error_response = {
        {"status", "error"},
        {"message", message}
    };

    res.body() = error_response.dump();
    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return res;
  }

  /**
   * Create a bad request response with a given message.
   * @param message Message to include in the response.
   * @param req Request that caused the error.
   * @return Response with the given message.
   */
  http::response<http::string_body> make_bad_request_response(
    const std::string& message,  const http::request<http::string_body>& req) {
    
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, "Beast");
    res.set(http::field::content_type, "application/json");
    
    nlohmann::json error_response = {
        {"status", "error"},
        {"message", message}
    };
    
    res.body() = error_response.dump();
    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return res;
  }

  /**
   * Create an OK request response with a given message.
   * @param message Message to include in the response.
   * @param req Request that caused the error.
   * @return Response with the given message.
   */
  http::response<http::string_body> make_ok_request_response(
    const std::string& message, const http::request<http::string_body>& req) {
    
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "Beast");
    res.set(http::field::content_type, "application/json");

    auto message_json = nlohmann::json::parse(message);
    nlohmann::json ok_response = {
        {"status", "ok"},
        {"message", message_json}
    };

    res.body() = ok_response.dump();
    res.keep_alive(req.keep_alive());
    res.prepare_payload();

    return res;
  }
}