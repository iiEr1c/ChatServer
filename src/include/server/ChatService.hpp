#pragma once

#include "common/redis/redis.hpp"
#include "model/friendModel.hpp"
#include "model/offlineMsgModel.hpp"
#include "model/userModel.hpp"
#include "model/groupModel.hpp"
#include "model/offlineGroupMsgModel.hpp"
#include "server/ChatTcpConn.hpp"

#include <fmt/core.h>

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

struct ChatService {
  static ChatService &instance() {
    static ChatService service;
    return service;
  }

  auto &getHandler(int id) {
    // 假定存在F
    return msgHandler_[id];
  }

  void reg(const std::shared_ptr<ChatTcpConn> &);
  void login(const std::shared_ptr<ChatTcpConn> &);
  void logout(const std::shared_ptr<ChatTcpConn> &);
  void heartbeat(const std::shared_ptr<ChatTcpConn> &);
  void p2pchat(const std::shared_ptr<ChatTcpConn> &);
  void p2pRecvMsgACK(const std::shared_ptr<ChatTcpConn> &);
  void closeConn(const std::shared_ptr<ChatTcpConn> &);

  void groupChat(const std::shared_ptr<ChatTcpConn> &);
  void groupChatRecvMsgACK(const std::shared_ptr<ChatTcpConn> &);

  // 添加好友请求
  void addFriendReq(const std::shared_ptr<ChatTcpConn> &);
  // 当对方同意时才往数据库加入
  void handleFriendReq(const std::shared_ptr<ChatTcpConn> &);

  void handleRedisSubscribeMsg(int userid, std::string msg);

  
  // 群聊
  void createGroup(const std::shared_ptr<ChatTcpConn> &);

protected:
  ChatService();

private:
  ChatService(const ChatService &) = delete;
  ChatService &operator=(const ChatService &) = delete;

  // 如果返回为nulptr则不在同个server上或者offline
  std::shared_ptr<ChatTcpConn> checkSameServerAndOnline(int userid);
  User queryUser(int userid);
  bool checkOnline(int userid);

private:
  // 每一种消息对应的handler
  std::unordered_map<int,
                     std::function<void(const std::shared_ptr<ChatTcpConn> &)>>
      msgHandler_;

  std::mutex mutex_;
  std::unordered_map<int32_t, std::weak_ptr<ChatTcpConn>> uids_;

  UserModel userModel_;
  FriendModel friendModel_;
  OfflineMsgModel offlineMsgModel_;
  GroupModel groupModel_;
  OfflineGroupMsgModel offlineGroupMsgModel_;

  std::shared_ptr<IM::util::Redis> redis_;
};