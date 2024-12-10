#include "server.hpp"

int main() {
  try {
    auto const address = net::ip::make_address("0.0.0.0");
    unsigned short port = 8080;

    net::io_context ioc{1};
    auto listener = std::make_shared<server::Listener>(ioc, tcp::endpoint{address, port});
    std::cout << "Server started on " << address << ":" << port << std::endl;
    
    ioc.run();
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}