#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <map>
#include <time.h>

#include "parser/parser.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace server {
  std::map<std::string, std::string> parse_query_string(std::string_view query);
  http::response<http::string_body> make_bad_request_response(const std::string& message, const http::request<http::string_body>& req);
  http::response<http::string_body> handle_request(http::request<http::string_body> const& req);

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