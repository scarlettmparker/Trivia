#include "request.hpp"

namespace request {
    /**
   * Parse a query string into a map of key-value pairs
   * This allows query strings to be extracted from URLS, to be used as parameters.
   *
   * @param query Query string to parse.
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
   * Parse the category parameter from a request. This ensures that the category
   * parameter appears in the query string. If it does not, an empty optional is returned.
   *
   * @param req Request to parse the category from.
   */
  std::optional<std::string> parse_category_from_request(const http::request<http::string_body>& req) {
    std::string target_str(req.target());
    std::string_view target(target_str);
    auto query_pos = target.find('?');

    if (query_pos == std::string::npos) {
        std::cout << "No query string found in URL" << std::endl;
        return std::nullopt;
    }

    auto query = target.substr(query_pos + 1);
    auto params = parse_query_string(std::string(query));

    if (params.find("category_name") == params.end()) {
        std::cout << "Missing category parameter" << std::endl;
        return std::nullopt;
    }

    return params["category_name"];
  }

  /**
   * -------------- ENDPOINTS AND REQUESTS --------------
   */
  
  /**
   * Create a bad request response with a given message.
   * @param message Message to include in the response.
   * @param req Request that caused the error.
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