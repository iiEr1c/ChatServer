#pragma once

#include "ChatProtocol.hpp"
#include "ChatService.hpp"

#include <fmt/core.h>

#include <boost/asio.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

using namespace boost::asio::ip;

struct ChatClient : public std::enable_shared_from_this<ChatClient> {

  friend struct ChatService;

  ChatClient(boost::asio::io_context &io_context,
             const tcp::resolver::results_type &endpoint)
      : io_context_(io_context), socket_(io_context) {
    do_connect(endpoint);
  }

  void write(std::string data) {
    boost::asio::post(io_context_, [this, data = std::move(data),
                                    self = shared_from_this()]() mutable {
      // 将要写入的数据塞入队列中
      // todo: thread safe queue
      bool isEmpty = false;
      {
        std::lock_guard lock(mutex_);
        // 那么说明之前有任务在write
        isEmpty = write_messages_.empty();
        write_messages_.push_back(std::move(data));
      }
      // 如果没有正在执行的异步任务, 那么启动异步任务向socket写入数据
      if (isEmpty) {
        do_write();
      }
    });
  }

  void close() {
    if (has_close_.test()) {
      return;
    }
    boost::asio::post(socket_.get_executor(),
                      [this, self = shared_from_this()]() { socket_.close(); });
    has_close_.test_and_set();
  }

private:
  void do_connect(const tcp::resolver::results_type &endpoints) {
    boost::asio::async_connect(
        socket_, endpoints,
        [this](boost::system::error_code ec, tcp::endpoint peer) {
          if (!ec) {
            // ...
            fmt::print("start parse_header().........\n");
            parse_header();
          }
        });
  }

  void parse_header() {
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(read_message_.data(),
                            ChatProtocol::GetHeaderLength()),
        [this, self = shared_from_this()](boost::system::error_code ec,
                                          std::size_t length) {
          if (!ec) {
            const ChatProtocol *header = reinterpret_cast<const ChatProtocol *>(
                self->read_message_.data());
            if (header->isValid()) {
              parse_body(header->bodyLen_);
            } else {
              self->close();
            }
          } else {
            // if (ec == boost::asio::error::eof && length > 0) {
            //   // response
            // }
            fmt::print("eof......................\n");
            close();
          }
        });
  }

  void parse_body(size_t bodyLen) {
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(
            read_message_.data() + ChatProtocol::GetHeaderLength(), bodyLen),
        [this, self = shared_from_this()](boost::system::error_code ec,
                                          std::size_t length) {
          if (!ec) {
            // 业务逻辑
            const ChatProtocol *header =
                reinterpret_cast<const ChatProtocol *>(read_message_.data());
            fmt::print("header->serviceId_ = {}\n", header->serviceId_);
            ChatService::instance().getHandler(header->serviceId_)(self);
            //  wait next header and body
            parse_header();
          } else {
            close();
          }
        });
  }

private:
  void do_write() {
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(write_messages_.front().data(),
                            write_messages_.front().size()),
        [this, self = shared_from_this()](boost::system::error_code ec,
                                          std::size_t /*length*/) {
          if (!ec) {
            bool isEmpty = false;
            {
              std::lock_guard lock(mutex_);
              write_messages_.pop_front();
              isEmpty = write_messages_.empty();
            }
            if (!isEmpty) {
              do_write();
            }
          } else {
            close();
          }
        });
  }

private:
  boost::asio::io_context &io_context_;
  std::atomic_flag has_close_ = false;
  tcp::socket socket_;
  std::mutex mutex_;
  std::array<char, 1024> read_message_;
  std::deque<std::string> write_messages_;
};
