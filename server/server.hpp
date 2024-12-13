#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>

#include <filesystem>
#include <dlfcn.h>
#include <vector>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <map>

#include "parser/parser.hpp"
#include "request/request_handler.hpp"
#include "request/postgres.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace server {
  std::vector<std::unique_ptr<RequestHandler>> load_handlers(const std::string& directory);
  http::response<http::string_body> handle_request(http::request<http::string_body> const& req, const std::string& ip_address);

  class Session : public std::enable_shared_from_this<Session> {
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;

  public:
    explicit Session(tcp::socket socket);
    void run();

  private:
    void do_read();
    void do_write(http::response<http::string_body> res);
  };

  class Listener : public std::enable_shared_from_this<Listener> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

  public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint);
    void do_accept();
  };
}

#endif