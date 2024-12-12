#include "server.hpp"

namespace server {
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

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req) {
    if (req.target().starts_with("/api/category")) {
      /**
       * -------------- GET CATEGORY --------------
       */

      if (req.method() == http::verb::get) {
        auto category_opt = parse_category_from_request(req);
        if (!category_opt) {
            return make_bad_request_response("Invalid category parameters", req);
        }

        std::string category = *category_opt;
        std::cout << "Category requested: " << category << std::endl;
        parser::Category cat = parser::parse_category("../questions/", category.c_str());
        if (strncmp(cat.category, "NO_CATEGORY", 10) == 0) {
            return make_bad_request_response("Category not found", req);
        }

        return make_ok_request_response(parser::fetch_category(cat).dump(), req);
      } else if (req.method() == http::verb::put) {
        /**
         * -------------- PUT NEW CATEGORY --------------
         */

        auto json_request = nlohmann::json::object();
        try {
          json_request = nlohmann::json::parse(req.body());
        } catch (const nlohmann::json::parse_error& e) {
          return make_bad_request_response("Invalid JSON request", req);
        }

        if (!json_request.contains("category_name")) {
          return make_bad_request_response("Invalid request: Missing required field (category_name).", req);
        }

        nlohmann::json response_json;
        if (!(postgres::create_category(json_request["category_name"].get<std::string>().c_str()))) {
          return make_bad_request_response("Category already exists", req);
        }

        response_json["message"] = "Category created successfully";
        response_json["category"] = json_request["category_name"];
        return make_ok_request_response(response_json.dump(4), req);
      } else if (req.method() == http::verb::delete_) {
        /**
         * -------------- DELETE CATEGORY --------------
         */

        auto category_opt = parse_category_from_request(req);
        if (!category_opt) {
          return make_bad_request_response("Invalid category parameters", req);
        }

        std::string category = *category_opt;
        nlohmann::json response_json;
        if (postgres::delete_category(category.c_str())) {
          response_json["message"] = "Category deleted successfully";
          response_json["category_name"] = category;
          return make_ok_request_response(response_json.dump(4), req);
        } else {
          return make_bad_request_response("Category not found", req);
        }
      } else {
        return make_bad_request_response("Invalid method", req);
      }
    }

    return make_bad_request_response("Invalid endpoint", req);
  }

  Session::Session(tcp::socket socket) : socket_(std::move(socket)) {}
  void Session::run() {
    do_read();
  }

  void Session::do_read() {
    auto self(shared_from_this());
    http::async_read(socket_, buffer_, req_, [this, self](beast::error_code ec, std::size_t) {
      if (!ec) {
        do_write(handle_request(req_));
      }
    });
  }

  void Session::do_write(http::response<http::string_body> res) {
    auto self(shared_from_this());
    auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));
    http::async_write(socket_, *sp, [this, self, sp](beast::error_code ec, std::size_t) {
      socket_.shutdown(tcp::socket::shutdown_send, ec);
    });
  }

  Listener::Listener(net::io_context& ioc, tcp::endpoint endpoint)
      : ioc_(ioc), acceptor_(net::make_strand(ioc)) {
    beast::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      std::cerr << "Open error: " << ec.message() << std::endl;
      return;
    }

    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
      std::cerr << "Set option error: " << ec.message() << std::endl;
      return;
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
      std::cerr << "Bind error: " << ec.message() << std::endl;
      return;
    }

    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
      std::cerr << "Listen error: " << ec.message() << std::endl;
      return;
    }

    do_accept();
  }

  void Listener::do_accept() {
    acceptor_.async_accept(net::make_strand(ioc_), [this](beast::error_code ec, tcp::socket socket) {
      if (!ec) {
        std::make_shared<Session>(std::move(socket))->run();
      }
      do_accept();
    });
  }

}