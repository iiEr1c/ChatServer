#include "server/ChatServer.hpp"
#include "common/ChatProtocol.hpp"
#include "server/ChatService.hpp"
#include "server/ChatTcpConn.hpp"

#include <fmt/core.h>

#include <memory>

ChatServer::ChatServer(boost::asio::io_context &context, short port)
    : acceptor_(context, boost::asio::ip::tcp::endpoint(
                             boost::asio::ip::tcp::v4(), port)) {
  // 启动服务器
  do_accept();
}

void ChatServer::close() {
  boost::asio::post(acceptor_.get_executor(), [this]() {
    boost::system::error_code ec;
    acceptor_.cancel(ec);
    if (ec) {
      // ...
    }
    acceptor_.close(ec);
    if (ec) {
      // ...
    }
  });
}

void ChatServer::do_accept() {
  acceptor_.async_accept([this](boost::system::error_code ec,
                                boost::asio::ip::tcp::socket socket) {
    if (!acceptor_.is_open()) {
      return;
    }
    if (!ec) {
      // 设置对应的回调
      auto ptr = std::make_shared<ChatTcpConn>(
          std::move(socket),
          [this](const std::shared_ptr<ChatTcpConn> &conn) {
            parse_header(conn);
          },
          [this](const std::shared_ptr<ChatTcpConn> &conn) {
            handle_disconnect(conn);
          });
      if (ptr != nullptr) {
        fmt::print("xxxxxxxxxxxx\n");
        ptr->refresh_heartbeat(std::chrono::seconds(60));
        ptr->check_heartbeat();
        ptr->start();
      }
    }

    do_accept();
  });
}

void ChatServer::parse_header(const std::shared_ptr<ChatTcpConn> &conn) {
  boost::asio::async_read(
      conn->socket_,
      boost::asio::buffer(conn->inMsgData_.data(),
                          ChatProtocol::GetHeaderLength()),
      [this, self = conn](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
          const ChatProtocol *header =
              reinterpret_cast<const ChatProtocol *>(self->inMsgData_.data());
          if (header->isValid()) {
            parse_body(self, header->bodyLen_);
          } else {
            self->close();
          }
        } else {
          // if (ec == boost::asio::error::eof && length > 0) {
          //   // response
          // }
          fmt::print("eof......................\n");
          self->close();
        }
      });
}

void ChatServer::parse_body(const std::shared_ptr<ChatTcpConn> &conn,
                            size_t bodyLen) {
  boost::asio::async_read(
      conn->socket_,
      boost::asio::buffer(
          conn->inMsgData_.data() + ChatProtocol::GetHeaderLength(), bodyLen),
      [this, self = conn](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
          // 业务逻辑
          const ChatProtocol *header =
              reinterpret_cast<const ChatProtocol *>(self->inMsgData_.data());
          fmt::print("header->serviceId_ = {}\n", header->serviceId_);
          ChatService::instance().getHandler(header->serviceId_)(self);
          // wait next header and body
          parse_header(self);
        } else {
          self->close();
        }
      });
}

void ChatServer::handle_disconnect(const std::shared_ptr<ChatTcpConn> &conn) {
  /*
   * when client disconnect
   * 1. update user's state to offline
   * 2. disconnect redis's connection?
   * but, where is every connection's context?
   */
  ChatService::instance().closeConn(conn);
}