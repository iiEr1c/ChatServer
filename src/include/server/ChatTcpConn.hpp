#pragma once

#include "common/ChatProtocol.hpp"

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include <array>
#include <chrono>
#include <concepts>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>

using boost::asio::ip::tcp;

struct ChatServer;
struct ChatService;

struct ChatTcpConn : public std::enable_shared_from_this<ChatTcpConn> {

  struct context {
    int uid;
    int status; // 0=offline, 1=online
  };

  friend class ChatServer;
  friend class ChatService;

  using onMsgCallback =
      std::function<void(const std::shared_ptr<ChatTcpConn> &)>;
  using onDisconnectCallback =
      std::function<void(const std::shared_ptr<ChatTcpConn> &)>;

  ChatTcpConn(tcp::socket socket, onMsgCallback, onDisconnectCallback);

  void start();

  void send(std::string msg);

  template <typename T>
  requires(std::is_same_v<std::decay_t<T>, std::chrono::seconds> ||
           std::is_same_v<std::decay_t<T>, std::chrono::milliseconds>)
  void refresh_heartbeat(T &&timeout) {
    heartbeatTimeout_.expires_after(std::forward<T>(timeout));
  }

private:
  void close();

  void do_write();

  void check_heartbeat();

private:
  tcp::socket socket_;
  std::atomic_flag has_close_ = false;
  std::function<void(std::shared_ptr<ChatTcpConn>)> onMessage;
  std::function<void(std::shared_ptr<ChatTcpConn>)> onDisconnect;

  boost::asio::steady_timer heartbeatTimeout_{socket_.get_executor()};

  std::mutex mutex_;
  std::deque<std::string> outMsg_;
  // todo: buffer
  std::array<char, 2048> inMsgData_;

  context context_; // 业务上下文
};
