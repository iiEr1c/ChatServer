#pragma once

#include <boost/asio.hpp>

struct ChatTcpConn;

struct ChatServer {
  ChatServer(boost::asio::io_context &, short port);
  void close();

private:
  void do_accept();

  void parse_header(const std::shared_ptr<ChatTcpConn> &);
  void parse_body(const std::shared_ptr<ChatTcpConn> &, size_t);

  void handle_disconnect(const std::shared_ptr<ChatTcpConn> &);

private:
  boost::asio::ip::tcp::acceptor acceptor_;
};