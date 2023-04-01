#include "Register.pb.h"

#include <fmt/core.h>

#include "common/ChatProtocol.hpp"
#include "server/ChatTcpConn.hpp"

#include <cassert>

ChatTcpConn::ChatTcpConn(tcp::socket socket, onMsgCallback onMsgCb,
                         onDisconnectCallback onDisConnCb)
    : socket_(std::move(socket)), onMessage(std::move(onMsgCb)),
      onDisconnect(std::move(onDisConnCb)) {}

void ChatTcpConn::start() {
  assert(onMessage != nullptr);
  onMessage(shared_from_this());
}

void ChatTcpConn::send(std::string msg) {
  bool isEmpty = false;
  {
    std::lock_guard lock(mutex_);
    isEmpty = outMsg_.empty();
    outMsg_.emplace_back(std::move(msg));
  }
  if (isEmpty) {
    do_write();
  }
}

void ChatTcpConn::check_heartbeat() {
  heartbeatTimeout_.async_wait(
      [this, self = shared_from_this()](boost::system::error_code ec) {
        // todo: maybe race?
        if (!socket_.is_open()) {
          return;
        }

        if (heartbeatTimeout_.expiry() <=
            boost::asio::steady_timer::clock_type::now()) {
          close();
        } else {
          check_heartbeat();
        }
      });
}

void ChatTcpConn::close() {
  if (has_close_.test()) {
    return;
  }
  /**
   * todo:
   * 1. 处理write没有发送完就close的问题
   * 2. 处理当我们想要close, 但是另一个线程仍然在获取消息然后发送的问题
   */
  boost::asio::post(socket_.get_executor(), [this, self = shared_from_this()] {
    // socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    // 执行关闭连接前的回调
    assert(onDisconnect != nullptr);
    onDisconnect(self);
    socket_.close();
  });
  has_close_.test_and_set();
}

void ChatTcpConn::do_write() {
  boost::asio::async_write(
      socket_,
      boost::asio::buffer(outMsg_.front().data(), outMsg_.front().size()),
      [this, self = shared_from_this()](boost::system::error_code ec,
                                        std::size_t length) {
        if (!ec) {
          fmt::print("server already send {} bytes to client\n", length);
          bool isEmpty = false;
          {
            std::lock_guard lock(mutex_);
            outMsg_.pop_front();
            isEmpty = outMsg_.empty();
          }
          if (!isEmpty) {
            do_write();
          }
        } else {
          self->close();
        }
      });
}