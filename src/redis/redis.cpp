#include "common/redis/redis.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <cassert>
#include <fstream>
#include <iostream>
#include <thread>

using json = nlohmann::json;

namespace IM {
namespace util {
Redis::Redis() {}

Redis::~Redis() {
  if (m_publish_ctx != nullptr) {
    ::redisFree(m_publish_ctx);
  }
  if (m_subscribe_ctx != nullptr) {
    ::redisFree(m_subscribe_ctx);
  }
}

bool Redis::connect(const std::string &confPath) {
  /*
      {
          "ip": "127.0.0.1",
          "port": 6379
      }
  */
  std::ifstream confFile(confPath);
  if (confFile.is_open()) {
    json conf;
    confFile >> conf;
    m_publish_ctx = ::redisConnect(conf["ip"].get<std::string>().c_str(),
                                   conf["port"].get<unsigned int>());
    m_subscribe_ctx = ::redisConnect(conf["ip"].get<std::string>().c_str(),
                                     conf["port"].get<unsigned int>());
    if (m_publish_ctx != nullptr && m_subscribe_ctx != nullptr) {
      if (m_publish_ctx->err || m_subscribe_ctx->err) {
        return false;
      }
      /* 启动一个线程监听事件 */
      std::thread([self = shared_from_this()] {
        self->consume_channel_message();
      }).detach();

      fmt::print("connect to redis server: {}:{}\n",
                 conf["ip"].get<std::string>(),
                 conf["port"].get<unsigned int>());
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool Redis::publish(int channel, const std::string &msg) {
  redisReply *reply = static_cast<redisReply *>(
      ::redisCommand(m_publish_ctx, "PUBLISH %d %s", channel, msg.c_str()));
  if (reply == nullptr) {
    return false;
  }
  ::freeReplyObject(reply);
  return true;
}

bool Redis::subscribe(int channel) {
  /* 只订阅, 不接收消息 */
  if (REDIS_ERR ==
      ::redisAppendCommand(m_subscribe_ctx, "SUBSCRIBE %d", channel)) {
    std::cout << "failed to subscribe\n";
    return false;
  }
  int done = 0;
  while (!done) {
    if (REDIS_ERR == ::redisBufferWrite(m_subscribe_ctx, &done)) {
      std::cout << "failed to subscribe command\n";
      return false;
    }
  }

  return true;
}

bool Redis::unsubscribe(int channel) {
  /* 取消订阅 */
  if (REDIS_ERR ==
      ::redisAppendCommand(m_subscribe_ctx, "UNSUBSCRIBE %d", channel)) {
    std::cout << "failed to unsubscribe\n";
    return false;
  }
  int done = 0;
  while (!done) {
    if (REDIS_ERR == ::redisBufferWrite(m_subscribe_ctx, &done)) {
      std::cout << "failed to subscribe command\n";
      return false;
    }
  }

  return true;
}

void Redis::set_notify_handler(
    std::function<void(int, const std::string &)> fn) {
  m_nofity_message_handler = std::move(fn);
}

void Redis::consume_channel_message() {
  redisReply *reply = nullptr;
  while (REDIS_OK ==
         redisGetReply(m_subscribe_ctx, reinterpret_cast<void **>(&reply))) {
    if (reply != nullptr && reply->element[2] != nullptr &&
        reply->element[2]->str != nullptr) {
      assert(m_nofity_message_handler != nullptr);
      std::cout << "reply->elements = " << reply->elements << '\n';
      for (size_t i = 0; i < reply->elements; ++i) {
        reply->element[i]->vtype;
        std::cout << "len = " << reply->element[i]->len
                  << ",str = " << reply->element[i]->str << '\n';
      }
      // todo: using std::string_view replace std::string
      m_nofity_message_handler(atoi(reply->element[1]->str),
                               reply->element[2]->str);
    }

    ::freeReplyObject(reply);
  }

  std::cout << "consume_channel_message exit\n";
}

} // namespace util
} // namespace IM