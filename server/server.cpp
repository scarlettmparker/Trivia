#include "server.hpp"

namespace server {
  std::vector<std::unique_ptr<RequestHandler>> load_handlers(const std::string& directory) {
    std::vector<std::unique_ptr<RequestHandler>> handlers;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
      if (entry.path().extension() == ".so") {
        void* lib_handle = dlopen(entry.path().c_str(), RTLD_LAZY);
        if (!lib_handle) {
          std::cerr << "Failed to load library: " << entry.path().string() << " - " << dlerror() << std::endl;
          throw std::runtime_error("Failed to load library: " + entry.path().string());
        }

        auto create_handler = reinterpret_cast<RequestHandler* (*)()>(dlsym(lib_handle, "create_handler"));
        if (!create_handler) {
          dlclose(lib_handle);
          throw std::runtime_error("Failed to find create_handler function in: " + entry.path().string());
        }
        handlers.emplace_back(create_handler());
      }
    }
    return handlers;
  }

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req) {
  static auto handlers = load_handlers("../api");

  for (const auto& handler : handlers) {
    std::cout << "Checking handler for endpoint: " << handler->get_endpoint() << std::endl;
    if (req.target().starts_with(handler->get_endpoint())) {
      return handler->handle_request(req);
    }
  }

  std::cerr << "No handler found for endpoint: " << req.target() << std::endl;
  return {http::status::not_found, req.version()};
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