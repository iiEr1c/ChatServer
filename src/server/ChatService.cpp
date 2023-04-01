#include "server/ChatService.hpp"
#include "common/ChatProtocol.hpp"

#include "AddFriend.pb.h"
#include "AddFriendACK.pb.h"
#include "CreateGroup.pb.h"
#include "CreateGroupACK.pb.h"
#include "GroupChat.pb.h"
#include "GroupChatACK.pb.h"
#include "HandleFriendReq.pb.h"
#include "HandleFriendReqACK.pb.h"
#include "Heartbeat.pb.h"
#include "HeartbeatACK.pb.h"
#include "Login.pb.h"
#include "LoginACK.pb.h"
#include "Logout.pb.h"
#include "LogoutACK.pb.h"
#include "P2PChatMsg.pb.h"
#include "P2PChatMsgACK.pb.h"
#include "Register.pb.h"
#include "RegisterACK.pb.h"

#include <fmt/core.h>

#include <cassert>

ChatService::ChatService() {
  msgHandler_.insert(
      {MsgServiceId::MSG_REG,
       [this](const std::shared_ptr<ChatTcpConn> &conn) { reg(conn); }});
  msgHandler_.insert(
      {MsgServiceId::MSG_LOGIN,
       [this](const std::shared_ptr<ChatTcpConn> &conn) { login(conn); }});
  msgHandler_.insert(
      {MsgServiceId::MSG_LOGOUT,
       [this](const std::shared_ptr<ChatTcpConn> &conn) { logout(conn); }});
  msgHandler_.insert({MsgServiceId::MSG_ADD_FRIEND,
                      [this](const std::shared_ptr<ChatTcpConn> &conn) {
                        addFriendReq(conn);
                      }});
  msgHandler_.insert(
      {MsgServiceId::MSG_HEARTBEAT,
       [this](const std::shared_ptr<ChatTcpConn> &conn) { heartbeat(conn); }});
  msgHandler_.insert(
      {MsgServiceId::P2PMSG_CLIENT_TO_SERVER,
       [this](const std::shared_ptr<ChatTcpConn> &conn) { p2pchat(conn); }});
  msgHandler_.insert({MsgServiceId::P2PMSG_SERVER_TO_CLIENT_ACK,
                      [this](const std::shared_ptr<ChatTcpConn> &conn) {
                        p2pRecvMsgACK(conn);
                      }});

  msgHandler_.insert({MsgServiceId::GROUPMSG_SERVER_TO_CLIENT_ACK,
                      [this](const std::shared_ptr<ChatTcpConn> &conn) {
                        groupChatRecvMsgACK(conn);
                      }});

  msgHandler_.insert({MsgServiceId::MSG_ADD_FRIEND_RES,
                      [this](const std::shared_ptr<ChatTcpConn> &conn) {
                        handleFriendReq(conn);
                      }});

  msgHandler_.insert(
      {MsgServiceId::GROUPMSG_CLIENT_TO_SERVER,
       [this](const std::shared_ptr<ChatTcpConn> &conn) { groupChat(conn); }});

  msgHandler_.insert({MsgServiceId::MSG_CREATE_GROUP,
                      [this](const std::shared_ptr<ChatTcpConn> &conn) {
                        createGroup(conn);
                      }});

  redis_ = std::make_shared<IM::util::Redis>();
  if (redis_->connect("/home/er1c/code/IM/conf/redis_conf.json")) {
    redis_->set_notify_handler([this](int userid, std::string msg) {
      handleRedisSubscribeMsg(userid, std::move(msg));
    });
  }
}

void ChatService::reg(const std::shared_ptr<ChatTcpConn> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->inMsgData_.data());
  bool succ = false;
  Register regInfo;
  auto parseRet = regInfo.ParseFromArray(conn->inMsgData_.data() +
                                             ChatProtocol::GetHeaderLength(),
                                         header->bodyLen_);
  if (parseRet) {
    conn->refresh_heartbeat(std::chrono::seconds(60));

    User user{.name = regInfo.name(),
              .password = regInfo.password(),
              .state = "offline"};
    RegisterACK regACK;
    if (userModel_.insert(user)) {
      regACK.set_regresult(0);
      regACK.set_reason("注册成功");
    } else {
      regACK.set_regresult(1);
      regACK.set_reason("注册失败");
    }
    auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_REG_ACK,
                                       regACK.SerializeAsString());
    assert(msg.size() > ChatProtocol::GetHeaderLength());
    conn->send(std::move(msg));
  } else {
    conn->close();
  }
}

void ChatService::login(const std::shared_ptr<ChatTcpConn> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->inMsgData_.data());
  Login loginInfo;
  auto parseRet = loginInfo.ParseFromArray(conn->inMsgData_.data() +
                                               ChatProtocol::GetHeaderLength(),
                                           header->bodyLen_);
  LoginACK loginACK;
  if (parseRet) {
    conn->refresh_heartbeat(std::chrono::seconds(60));
    auto user = userModel_.query(loginInfo.id());
    if (user.id == loginInfo.id() && user.password == loginInfo.password()) {
      if (user.state == "online") {
        // 不需要重复登陆
        loginACK.set_loginresult(0);
        loginACK.set_reason("重复登陆");
        // 对于此次登录的响应
        auto ackStr = loginACK.SerializeAsString();
        assert(ackStr.size() > 0);
        conn->send(ChatProtocol::genPacket(MsgServiceId::MSG_LOGIN_ACK,
                                           std::move(ackStr)));
        // 设置上下文
        conn->context_.uid = user.id;
        conn->context_.status = 1;
        fmt::print("不要重复登录...........\n");
      } else {
        user.state = "online";
        userModel_.updateState(user);
        loginACK.set_loginresult(0);
        loginACK.set_reason(std::string("登录成功"));

        // 设置上下文
        conn->context_.uid = user.id;
        conn->context_.status = 1;

        // 处理redis
        // redis_->subscribe(user.id);

        // 插入对应的ptr
        {
          std::lock_guard lock(mutex_);
          uids_.insert_or_assign(user.id, conn);
        }
        // 更新tcp conn的业务场景上下文
        conn->context_ =
            ChatTcpConn::context{.uid = loginInfo.id(), .status = 1};
        // 返回离线消息
        auto offlineMsgs = offlineMsgModel_.query(user.id);
        if (!offlineMsgs.empty()) {
          fmt::print("handle offline messages..............\n");
          for (auto &&msg : offlineMsgs) {
            loginACK.add_offlinemsgs(std::move(msg));
          }
          // 清除离线消息
          offlineMsgModel_.remove(user.id);
        }

        // 返回好友列表
        auto friendList = friendModel_.query(user.id);
        for (auto &user : friendList) {
          auto *userInfo = loginACK.add_friendlists();
          userInfo->set_uid(user.id);
          userInfo->set_name(user.name);
          userInfo->set_status(user.state);
        }

        // 返回群聊信息

        // 对于此次登录的响应
        auto ackStr = loginACK.SerializeAsString();
        assert(ackStr.size() > 0);
        conn->send(ChatProtocol::genPacket(MsgServiceId::MSG_LOGIN_ACK,
                                           std::move(ackStr)));
      }
    } else {
      loginACK.set_loginresult(1);
      loginACK.set_reason("密码错误或用户不存在");

      conn->send(ChatProtocol::genPacket(MsgServiceId::MSG_LOGIN_ACK,
                                         loginACK.SerializeAsString()));
    }
  } else {
    conn->close();
  }
}

void ChatService::logout(const std::shared_ptr<ChatTcpConn> &conn) {
  /*
   * todo: 解决主动logout然后因为tcp断开二次调用问题(幂等)
   */
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->inMsgData_.data());
  Logout logoutInfo;
  auto parseRet = logoutInfo.ParseFromArray(conn->inMsgData_.data() +
                                                ChatProtocol::GetHeaderLength(),
                                            header->bodyLen_);
  LogoutACK logoutACK;
  if (parseRet) {
    if (conn->context_.status == 1 && conn->context_.uid == logoutInfo.id()) {
      // uids_清除对应的client id
      {
        std::lock_guard lock(mutex_);
        uids_.erase(conn->context_.uid);
      }
      User user{.id = conn->context_.uid, .state = "offline"};
      userModel_.updateState(user);
      conn->context_.status = 0;
      logoutACK.set_logoutresult(0);
      logoutACK.set_reason("success to quit");
    } else {
      logoutACK.set_logoutresult(1);
      logoutACK.set_reason("invalid request");
    }

    // 是否应该要respond呢?
    auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_LOGOUT_ACK,
                                       logoutACK.SerializeAsString());
    fmt::print("logoutACK: msg's size = {}\n", msg.size());
    conn->send(std::move(msg));

    // 连接何时close呢?
  } else {
    // 直接close?
    conn->close();
  }
}

// 其实就是更新数据库和redis的状态
void ChatService::closeConn(const std::shared_ptr<ChatTcpConn> &conn) {
  if (conn->context_.status == 1) {
    fmt::print("only client don't execute logout........\n");
    {
      std::lock_guard lock(mutex_);
      uids_.erase(conn->context_.uid);
    }
    User user{.id = conn->context_.uid, .state = "offline"};
    userModel_.updateState(user);
    conn->context_.status = 0;
  }
}

void ChatService::addFriendReq(const std::shared_ptr<ChatTcpConn> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->inMsgData_.data());
  AddFriend addFriendInfo;
  auto parseRet = addFriendInfo.ParseFromArray(
      conn->inMsgData_.data() + ChatProtocol::GetHeaderLength(),
      header->bodyLen_);
  if (parseRet) {
    conn->refresh_heartbeat(std::chrono::seconds(60));

    AddFriendACK ack;

    auto user = queryUser(addFriendInfo.toid());
    if (user.id != -1) {
      auto ptr = checkSameServerAndOnline(addFriendInfo.toid());
      if (ptr != nullptr) {
        // 对方在同一个服务器上
        HandleFriendReq handleFriendReq;
        handleFriendReq.set_fromid(addFriendInfo.fromid());
        handleFriendReq.set_toid(addFriendInfo.toid());
        handleFriendReq.set_addition(addFriendInfo.addition());
        auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_ADD_FRIEND_REQ,
                                           handleFriendReq.SerializeAsString());
        assert(msg.size() > ChatProtocol::GetHeaderLength());
        ptr->send(std::move(msg));

        ack.set_addfriendresult(0);
        ack.set_reason(
            fmt::format("已向用户{}发送好友请求", addFriendInfo.toid()));
      } else if (user.state == std::string("online")) {
        // todo
        // 在线but不同服务器
        ack.set_addfriendresult(1);
        ack.set_reason(fmt::format("todo: 暂时不支持跨服添加好友{}",
                                   addFriendInfo.toid()));
      } else {
        // todo
        // 离线
        ack.set_addfriendresult(1);
        ack.set_reason(
            fmt::format("todo: 用户{}离线状态", addFriendInfo.toid()));
      }
    } else {
      // 用户不存在
      ack.set_addfriendresult(1);
      ack.set_reason(fmt::format("用户{}不存在", addFriendInfo.toid()));
    }

    auto ackStr = ack.SerializeAsString();
    assert(ackStr.size() > 0);
    auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_ADD_FRIEND_ACK,
                                       std::move(ackStr));
    conn->send(std::move(msg));
  } else {
    conn->close();
  }
}

void ChatService::handleFriendReq(const std::shared_ptr<ChatTcpConn> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->inMsgData_.data());
  HandleFriendReqACK handleFriendRes;
  auto parseRet = handleFriendRes.ParseFromArray(
      conn->inMsgData_.data() + ChatProtocol::GetHeaderLength(),
      header->bodyLen_);
  if (parseRet) {
    conn->refresh_heartbeat(std::chrono::seconds(60));

    if (handleFriendRes.isagreed()) {
      // 同意添加
      auto ret =
          friendModel_.insert(handleFriendRes.fromid(), handleFriendRes.toid());
      assert(ret == true);

      auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_ADD_FRIEND_RES,
                                         handleFriendRes.SerializeAsString());
      assert(msg.size() > ChatProtocol::GetHeaderLength());
    } else {
      // 拒绝添加
      auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_ADD_FRIEND_RES,
                                         handleFriendRes.SerializeAsString());
      assert(msg.size() > ChatProtocol::GetHeaderLength());
    }
  } else {
    conn->close();
  }
}

void ChatService::createGroup(const std::shared_ptr<ChatTcpConn> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->inMsgData_.data());
  CreateGroup createGroupInfo;
  auto parseRet = createGroupInfo.ParseFromArray(
      conn->inMsgData_.data() + ChatProtocol::GetHeaderLength(),
      header->bodyLen_);
  if (parseRet) {
    conn->refresh_heartbeat(std::chrono::seconds(60));

    Group group{.name = createGroupInfo.groupname(),
                .desc = createGroupInfo.groupdesc()};
    bool createStatus = groupModel_.createGroup(group);
    assert(createStatus == true);
    assert(group.id > 0);
    // 创建者成为群主
    bool addGroupStatus = groupModel_.addGroup(createGroupInfo.creator(),
                                               group.id, GroupUser::Role::HOST);
    assert(addGroupStatus == true);
    CreateGroupACK ack;
    if (createStatus && addGroupStatus) {
      ack.set_groupid(group.id);
    } else {
      ack.set_groupid(-1);
    }

    auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_CREATE_GROUP_ACK,
                                       ack.SerializeAsString());
    assert(msg.size() > ChatProtocol::GetHeaderLength());
    conn->send(std::move(msg));
  } else {
    conn->close();
  }
}

void ChatService::heartbeat(const std::shared_ptr<ChatTcpConn> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->inMsgData_.data());
  Heartbeat HeartbeatInfo;
  auto parseRet = HeartbeatInfo.ParseFromArray(
      conn->inMsgData_.data() + ChatProtocol::GetHeaderLength(),
      header->bodyLen_);
  if (parseRet) {
    conn->refresh_heartbeat(std::chrono::seconds(60));

    HeartbeatACK ack;
    ack.set_random(HeartbeatInfo.random());
    auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_HEARTBEAT_ACK,
                                       ack.SerializeAsString());
    assert(msg.size() > ChatProtocol::GetHeaderLength());
    conn->send(std::move(msg));
  } else {
    conn->close();
  }
}

void ChatService::p2pchat(const std::shared_ptr<ChatTcpConn> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->inMsgData_.data());
  P2PChatMsg p2pChatMsg;
  auto parseRet = p2pChatMsg.ParseFromArray(conn->inMsgData_.data() +
                                                ChatProtocol::GetHeaderLength(),
                                            header->bodyLen_);
  if (parseRet) {
    conn->refresh_heartbeat(std::chrono::seconds(60));

    /**
     * todo: 判断fromid是否在线, toid是否是它的好友
     */

    int32_t toid = p2pChatMsg.toid();
    std::shared_ptr<ChatTcpConn> peerConn;
    {
      std::lock_guard lock{mutex_};
      if (auto it = uids_.find(toid); it != uids_.end()) {
        peerConn = it->second.lock();
      }
    }

    P2PChatMsgACK chatACK;
    if (peerConn != nullptr) {
      // 同服务器&在线, 换个header然后发给接收方
      // todo: 多次内存拷贝
      auto msg = ChatProtocol::genPacket(MsgServiceId::P2PMSG_SERVER_TO_CLIENT,
                                         p2pChatMsg.SerializeAsString());
      // 将消息发到对端
      peerConn->send(std::move(msg));

      chatACK.set_msgstatus(0);
      chatACK.set_reason("送达服务器");

      auto ack =
          ChatProtocol::genPacket(MsgServiceId::P2PMSG_CLIENT_TO_SERVER_ACK,
                                  chatACK.SerializeAsString());
      assert(ack.size() > ChatProtocol::GetHeaderLength());
      // 发送回执
      conn->send(std::move(ack));
      return;
    }

    auto user = userModel_.query(toid);
    if (user.id == -1) { // 用户不存在
      chatACK.set_msgstatus(1);
      chatACK.set_reason("用户不存在");

      auto ack =
          ChatProtocol::genPacket(MsgServiceId::P2PMSG_CLIENT_TO_SERVER_ACK,
                                  chatACK.SerializeAsString());
      assert(ack.size() > ChatProtocol::GetHeaderLength());
      // 发送回执
      conn->send(std::move(ack));
      return;
    }
    if (user.state == std::string("online")) {
      auto msg = ChatProtocol::genPacket(MsgServiceId::P2PMSG_SERVER_TO_CLIENT,
                                         p2pChatMsg.SerializeAsString());
      auto publishRet = redis_->publish(toid, msg);
      assert(publishRet == true);

      chatACK.set_msgstatus(0);
      chatACK.set_reason("消息发送至redis服务器");

      auto ack =
          ChatProtocol::genPacket(MsgServiceId::P2PMSG_CLIENT_TO_SERVER_ACK,
                                  chatACK.SerializeAsString());
      assert(ack.size() > ChatProtocol::GetHeaderLength());
      // 发送回执
      conn->send(std::move(ack));
      return;
    }

    // 用户处于离线状态, 存储在离线消息表中, 需要用户登录时将消息拉走
    // 消息内容为原内容, 并非protobuf格式的message
    offlineMsgModel_.insert(p2pChatMsg.fromid(), toid, p2pChatMsg.msg());
    chatACK.set_msgstatus(0);
    chatACK.set_reason("离线消息");
    auto ack = ChatProtocol::genPacket(
        MsgServiceId::P2PMSG_CLIENT_TO_SERVER_ACK, chatACK.SerializeAsString());
    assert(ack.size() > ChatProtocol::GetHeaderLength());
    // 发送回执
    conn->send(std::move(ack));
  } else {
    conn->close();
  }
}

void ChatService::p2pRecvMsgACK(const std::shared_ptr<ChatTcpConn> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->inMsgData_.data());
  P2PChatMsgACK msgACK;
  auto parseRet = msgACK.ParseFromArray(conn->inMsgData_.data() +
                                            ChatProtocol::GetHeaderLength(),
                                        header->bodyLen_);
  if (parseRet) {
    conn->refresh_heartbeat(std::chrono::seconds(60));

    // todo: 是否应该先发送方响应?
    fmt::print("接收消息响应.........\n");
  } else {
    conn->close();
  }
}

void ChatService::groupChat(const std::shared_ptr<ChatTcpConn> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->inMsgData_.data());
  GroupChat groupChatMsg;
  auto parseRet = groupChatMsg.ParseFromArray(
      conn->inMsgData_.data() + ChatProtocol::GetHeaderLength(),
      header->bodyLen_);
  if (parseRet) {
    // push模型
    fmt::print("void ChatService::groupChat...........................\n");
    auto userid = groupChatMsg.fromid();
    auto groupId = groupChatMsg.toid();
    auto otherMembers = groupModel_.queryGroupMember(groupId, userid);
    fmt::print("have {} member............\n", otherMembers.size());
    for (auto memberId : otherMembers) {
      auto ptr = checkSameServerAndOnline(memberId);
      if (ptr) {
        // 同一个server且在线
        auto msg =
            ChatProtocol::genPacket(MsgServiceId::GROUPMSG_SERVER_TO_CLIENT,
                                    groupChatMsg.SerializeAsString());
        assert(msg.size() > ChatProtocol::GetHeaderLength());
        conn->send(std::move(msg));
      } else {
        // 判断是否在线
        if (checkOnline(memberId)) {
          auto msg =
              ChatProtocol::genPacket(MsgServiceId::GROUPMSG_SERVER_TO_CLIENT,
                                      groupChatMsg.SerializeAsString());
          assert(msg.size() > ChatProtocol::GetHeaderLength());
          redis_->publish(memberId, msg);
        } else {
          // offline msg
          offlineGroupMsgModel_.insert(groupChatMsg.fromid(),
                                       groupChatMsg.toid(), memberId,
                                       groupChatMsg.msg());
        }
      }
    }

    // 向client响应
    GroupChatACK ack;
    ack.set_msgstatus(0);
    ack.set_reason("成功发出消息");
    auto msg = ChatProtocol::genPacket(
        MsgServiceId::GROUPMSG_CLIENT_TO_SERVER_ACK, ack.SerializeAsString());
    conn->send(msg);
  } else {
    conn->close();
  }
}

// 是否要ack呢, 这样子请求是否会非常大?
void ChatService::groupChatRecvMsgACK(
    const std::shared_ptr<ChatTcpConn> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->inMsgData_.data());
  GroupChatACK msgACK;
  auto parseRet = msgACK.ParseFromArray(conn->inMsgData_.data() +
                                            ChatProtocol::GetHeaderLength(),
                                        header->bodyLen_);
  if (parseRet) {
    conn->refresh_heartbeat(std::chrono::seconds(60));

    // todo: 是否应该先发送方响应?
    fmt::print("接收消息响应.........\n");
  } else {
    conn->close();
  }
}

void ChatService::handleRedisSubscribeMsg(int userid, std::string msg) {
  std::shared_ptr<ChatTcpConn> ptr;
  {
    std::lock_guard lock(mutex_);
    if (auto it = uids_.find(userid); it != uids_.end()) {
      ptr = it->second.lock();
    }
  }

  // 避免close时仍然继续发消息
  if (ptr != nullptr && !ptr->has_close_.test()) {
    ptr->send(std::move(msg));
    return;
  }

  // 处理离线消息
}

std::shared_ptr<ChatTcpConn> ChatService::checkSameServerAndOnline(int userid) {
  std::shared_ptr<ChatTcpConn> peerConn;
  {
    std::lock_guard lock{mutex_};
    if (auto it = uids_.find(userid); it != uids_.end()) {
      peerConn = it->second.lock();
    }
  }
  return peerConn;
}

bool ChatService::checkOnline(int userid) {
  auto user = userModel_.query(userid);
  return user.state == std::string("online");
}

User ChatService::queryUser(int userid) { return userModel_.query(userid); }