#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

struct ChatClient;

struct ChatService {
  static ChatService &instance() {
    static ChatService service;
    return service;
  }

  auto &getHandler(int id) {
    // 假定存在F
    return msgHandler_[id];
  }

  void regACK(const std::shared_ptr<ChatClient> &);
  void loginACK(const std::shared_ptr<ChatClient> &);
  void logoutACK(const std::shared_ptr<ChatClient> &);
  void addFriendACK(const std::shared_ptr<ChatClient> &);
  void handleAddFriendRes(const std::shared_ptr<ChatClient> &);
  void p2pSendMsgACK(const std::shared_ptr<ChatClient> &);
  void heartbeatACK(const std::shared_ptr<ChatClient> &);

  void
  p2pRecvMsg(const std::shared_ptr<ChatClient> &); // 接收来自服务器发来的msg

  void createGroupACK(const std::shared_ptr<ChatClient>);

protected:
  ChatService();

private:
  ChatService(const ChatService &) = delete;
  ChatService &operator=(const ChatService &) = delete;

private:
  // 每一种消息对应的handler
  std::unordered_map<int,
                     std::function<void(const std::shared_ptr<ChatClient> &)>>
      msgHandler_;
};