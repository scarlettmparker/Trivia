#include "server.hpp"

namespace server {
  /**
    * Load all request handlers from the specified directory.
    * This function loads all shared objects (.so files) from the specified directory and
    * looks for a function named create_<handler_name>_handler in each of them.
    *
    * @param directory Directory to load handlers from.
    * @return Vector of unique pointers to the loaded request handlers.
    */
  std::vector<std::unique_ptr<RequestHandler>> load_handlers(const std::string& directory) {
    std::vector<std::unique_ptr<RequestHandler>> handlers;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
      if (entry.path().extension() == ".so") {
        void* lib_handle = dlopen(entry.path().c_str(), RTLD_LAZY);
        if (!lib_handle) {
          throw std::runtime_error("Failed to load library: " + entry.path().string());
        }

        std::string filename = entry.path().stem().string();
        if (filename.rfind("lib", 0) == 0)
          filename = filename.substr(3);
        std::string function_name = "create_" + filename + "_handler";
        auto create_handler = reinterpret_cast<RequestHandler* (*)()>(dlsym(lib_handle, function_name.c_str()));
        if (!create_handler) {
          dlclose(lib_handle);
          throw std::runtime_error("Failed to find " + function_name + " function in: " + entry.path().string());
        }
        handlers.emplace_back(create_handler());
      }
    }

    return handlers;
  }

  /**
    * Handle an HTTP request. This function iterates over all loaded request handlers and
    * calls their handle_request method if the request target starts with the handler's endpoint.
    *
    * @param req HTTP request to handle.
    * @return HTTP response.
    */
  http::response<http::string_body> handle_request(http::request<http::string_body> const& req, const std::string& ip_address) {
    static auto handlers = load_handlers(".");
    http::response<http::string_body> res;

    // handle CORS preflight request
    if (req.method() == http::verb::options) {
        res = {http::status::no_content, req.version()};
        res.set(http::field::access_control_allow_origin, req["Origin"].to_string());
        res.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
        res.set(http::field::access_control_allow_headers, "Content-Type, Authorization, Access-Control-Allow-Origin");
        res.set(http::field::access_control_allow_credentials, "true");
        return res;
    }

    for (const auto& handler : handlers) {
        if (req.target().starts_with(handler->get_endpoint())) {
            res = handler->handle_request(req, ip_address);
            break;
        }
    }

    if (res.result() == http::status::unknown) {
        std::cerr << "No handler found for endpoint: " << req.target() << std::endl;
        res = {http::status::not_found, req.version()};
    }

    // set CORS headers
    res.set(http::field::access_control_allow_origin, req["Origin"].to_string());
    res.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
    res.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
    res.set(http::field::access_control_allow_credentials, "true");
    
    return res;
  }

  Session::Session(tcp::socket socket) : socket_(std::move(socket)) {}
  void Session::run() {
    do_read();
  }

  /**
   * Read a request from the client.
   */
  void Session::do_read() {
    auto self(shared_from_this());
    http::async_read(socket_, buffer_, req_, [this, self](beast::error_code ec, std::size_t) {
      if (!ec) {
        boost::asio::ip::address ip_address = socket_.remote_endpoint().address();
        std::string ip_str = ip_address.to_string();
        auto res = handle_request(req_, ip_str);
        do_write(std::move(res));
      }
    });
  }

  /**
   * Write a response to the client.
   * @param res Response to write.
   */
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

  /**
   * Start accepting incoming connections.
   */
  void Listener::do_accept() {
    acceptor_.async_accept(net::make_strand(ioc_), [this](beast::error_code ec, tcp::socket socket) {
      if (!ec) {
        std::make_shared<Session>(std::move(socket))->run();
      }
      do_accept();
    });
  }

}