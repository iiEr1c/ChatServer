#include <boost/asio.hpp>
#include <server/ChatServer.hpp>

#include <chrono>
#include <iostream>

#include <fmt/core.h>

int main() {
  boost::asio::io_context io_context;
  ChatServer server(io_context, 9999);
  io_context.run();

  auto p = std::chrono::seconds(10);
  return 0;
}