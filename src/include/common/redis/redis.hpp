#pragma once

#include <hiredis/hiredis.h>

#include <functional>
#include <memory>
#include <string>

namespace IM {
namespace util {
class Redis : public std::enable_shared_from_this<Redis> {
public:
  Redis();
  ~Redis();

  bool connect(const std::string &confPath);

  bool publish(int channel, const std::string &msg);

  bool subscribe(int channel);

  bool unsubscribe(int channel);

  Redis(const Redis &) = delete;
  Redis &operator=(const Redis &) = delete;

  void set_notify_handler(std::function<void(int, const std::string &)>);

private:
  void consume_channel_message();

private:
  redisContext *m_publish_ctx = nullptr;
  redisContext *m_subscribe_ctx = nullptr;

  std::function<void(int, const std::string &)> m_nofity_message_handler;
};
} // namespace util
} // namespace IM
