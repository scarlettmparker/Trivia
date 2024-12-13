#include <string>
#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

class RequestHandler {
public:
  virtual ~RequestHandler() = default;
  virtual std::string get_endpoint() const = 0;
  virtual http::response<http::string_body> handle_request(const http::request<http::string_body>& req, const std::string& ip_address) = 0;
};