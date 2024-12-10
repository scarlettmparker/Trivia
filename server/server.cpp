#include "server.hpp"

namespace server {
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

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req) {
    std::cout << req.target().substr(0, 14) << std::endl;
    if (req.method() == http::verb::get && req.target().substr(0, 16) == "/api/getcategory") {
      std::string target_str(req.target());
      auto target = std::string_view(target_str);
      auto query_pos = target.find('?');

      std::map<std::string, std::string> params;
      http::response<http::string_body> res{http::status::ok, req.version()};
      res.set(http::field::server, "Beast");
      res.set(http::field::content_type, "application/json");
        
      if (query_pos != std::string::npos) {
        auto query = target.substr(query_pos + 1);
        params = parse_query_string(query);
            
        if (params.find("category") != params.end()) {
          std::string category = params["category"];
          std::cout << "Category requested: " << category << std::endl;

          nlohmann::json json_response = {
            {"status", "success"},
            {"category", category},
            {"data", {{"id", 1}, {"name", category}, {"description", "Category description"}}}
          };
                
          res.body() = json_response.dump();
        } else {
          res.result(http::status::bad_request);
          nlohmann::json error_response = {
            {"status", "error"},
            {"message", "Missing category parameter"}
          };
          res.body() = error_response.dump();
        }
      } else {
        res.result(http::status::bad_request);
        nlohmann::json error_response = {
          {"status", "error"},
          {"message", "No query parameters provided"}
        };
        res.body() = error_response.dump();
      }
        
      res.keep_alive(req.keep_alive());
      res.prepare_payload();
      return res;
    }

    return http::response<http::string_body>{http::status::bad_request, req.version()};
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