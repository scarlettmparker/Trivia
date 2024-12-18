#include "request.hpp"

using namespace postgres;
namespace request {
  /* caching for session data */
  size_t MAX_CACHE_SIZE = 1000;
  int CACHE_TTL_SECONDS = 60;
  std::mutex cache_mutex;
  std::unordered_map<std::string_view, CachedUserData> session_cache;

  /**
   * Clean up the session cache by removing expired entries.
   */
  void cleanup_cache() {
    std::lock_guard<std::mutex> lock(request::cache_mutex);
    auto now = std::chrono::system_clock::now();
    
    for (auto it = request::session_cache.begin(); it != request::session_cache.end();) {
      if (it->second.expiry < now) {
        it = request::session_cache.erase(it);
      } else {
        ++it;
      }
    }

    // if cache is too large, remove the oldest entries
    if (request::session_cache.size() > request::MAX_CACHE_SIZE) {
      auto oldest = request::session_cache.begin();
      for (auto it = request::session_cache.begin(); it != request::session_cache.end(); ++it) {
        if (it->second.expiry < oldest->second.expiry) {
          oldest = it;
        }
      }
      request::session_cache.erase(oldest);
    }
  }

  /**
   * Invalidate a session by setting it to inactive.
   * @param session_id Session ID to invalidate.
   * @param verbose Whether to print messages to stdout.
   */
  void invalidate_session(const std::string_view& session_id, int verbose) {
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
   * Get the permissions for a user by their ID.
   * This is used to check if a user has the required permissions to access an endpoint.
   * @param user_id ID of the user to get the permissions for.
   * @param verbose Whether to print messages to stdout.
   * @return Array of permissions for the user.
   */
  UserPermissions get_user_permissions(int user_id, int verbose) {
    UserPermissions result = {0, nullptr};
    try {
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("get_user_permissions", user_id);
      txn.commit();

      if (r.empty()) {
        verbose && std::cerr << "User with ID " << user_id << " not found" << std::endl;
        return result;
      }

      result.permission_count = r.size();
      result.permissions = new Permission[r.size()];
      
      for (size_t i = 0; i < r.size(); i++) {
        result.permissions[i].id = r[i][0].as<int>();
        result.permissions[i].permission_name = r[i][1].c_str();
      }
      return result;
        
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }    
    return result;
  }

  /**
   * Get the session ID from a cookie in a request.
   * @param req Request to get the session ID from.
   * @return Session ID from the cookie.
   */
  std::string_view get_session_id_from_cookie(const http::request<http::string_body>& req) {
    auto cookie_iter = req.find(http::field::cookie);
    if (cookie_iter == req.end()) {
        return {};
    }
    const boost::string_view cookie = cookie_iter->value();
    if (cookie.empty()) {
        return {};
    }
    constexpr boost::string_view SESSION_KEY = "sessionId=";
    auto pos = cookie.find(SESSION_KEY);
    if (pos == std::string::npos) {
        return {};
    }
    pos += SESSION_KEY.length();
    auto end = cookie.find(';', pos);
    return std::string_view(cookie.data() + pos, 
      end == std::string::npos ? cookie.length() - pos : end - pos);
  }
  
  /**
   * Select the user data from the session ID.
   * @param session_id Session ID to select the user ID from.
   * @param verbose Whether to print messages to stdout.
   * @return User data if the session is valid, {-1, ""} otherwise.
   */
  UserData select_user_data_from_session(const std::string_view& session_id, int verbose) {
    {
      std::lock_guard<std::mutex> lock(request::cache_mutex);
      auto it = request::session_cache.find(session_id);
      if (it != request::session_cache.end()) {
        auto now = std::chrono::system_clock::now();
        if (it->second.expiry > now) {
          return {it->second.user_id, it->second.username};
        }
        request::session_cache.erase(it);
      }
    }

    try {
      pqxx::work txn(*postgres::c);
      pqxx::result r = txn.exec_prepared("select_user_data_from_session", session_id);
      txn.commit();

      if (r.empty()) {
        verbose && std::cerr << "Session ID " << session_id << " not found" << std::endl;
        request::invalidate_session(session_id, 0);
        return {-1, ""};
      }

      int user_id = std::stoi(r[0][0].c_str());
      std::string username = r[0][1].c_str();
      
      {
        std::lock_guard<std::mutex> lock(request::cache_mutex);
        auto expiry = std::chrono::system_clock::now() + std::chrono::seconds(request::CACHE_TTL_SECONDS);
        request::session_cache[session_id] = {user_id, username, expiry};
        
        if (request::session_cache.size() > request::MAX_CACHE_SIZE) {
          cleanup_cache();
        }
      }
      
      return {user_id, username};
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return {-1, ""};
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
        // std::cout << "No query string found in URL" << std::endl;
        return std::nullopt;
    }

    auto query = target.substr(query_pos + 1);
    auto params = parse_query_string(std::string(query));

    if (params.find(parameter) == params.end()) {
        // std::cout << "Missing category parameter" << std::endl;
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
   * Create a too many requests response with a given message.
   * @param message Message to include in the response.
   * @param req Request that caused the error.
   * @return Response with the given message.
   */
  http::response<http::string_body> make_too_many_requests_response(
    const std::string& message, const http::request<http::string_body>& req) {
    
    http::response<http::string_body> res{http::status::too_many_requests, req.version()};
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