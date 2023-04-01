#include "ChatService.hpp"
#include "ChatClient.hpp"

#include "AddFriendACK.pb.h"
#include "CreateGroupACK.pb.h"
#include "HandleFriendReq.pb.h"
#include "HeartbeatACK.pb.h"
#include "LoginACK.pb.h"
#include "LogoutACK.pb.h"
#include "P2PChatMsg.pb.h"
#include "P2PChatMsgACK.pb.h"
#include "RegisterACK.pb.h"

#include <fmt/core.h>

ChatService::ChatService() {
  msgHandler_.insert(
      {MsgServiceId::MSG_REG_ACK,
       [this](const std::shared_ptr<ChatClient> &conn) { regACK(conn); }});

  msgHandler_.insert(
      {MsgServiceId::MSG_LOGIN_ACK,
       [this](const std::shared_ptr<ChatClient> &conn) { loginACK(conn); }});

  msgHandler_.insert(
      {MsgServiceId::MSG_LOGOUT_ACK,
       [this](const std::shared_ptr<ChatClient> &conn) { logoutACK(conn); }});

  msgHandler_.insert({MsgServiceId::MSG_ADD_FRIEND_ACK,
                      [this](const std::shared_ptr<ChatClient> &conn) {
                        addFriendACK(conn);
                      }});

  msgHandler_.insert({MsgServiceId::MSG_HEARTBEAT_ACK,
                      [this](const std::shared_ptr<ChatClient> &conn) {
                        heartbeatACK(conn);
                      }});

  msgHandler_.insert({MsgServiceId::P2PMSG_CLIENT_TO_SERVER_ACK,
                      [this](const std::shared_ptr<ChatClient> &conn) {
                        p2pSendMsgACK(conn);
                      }});

  msgHandler_.insert({MsgServiceId::MSG_ADD_FRIEND_REQ,
                      [this](const std::shared_ptr<ChatClient> &conn) {
                        handleAddFriendRes(conn);
                      }});

  msgHandler_.insert(
      {MsgServiceId::P2PMSG_SERVER_TO_CLIENT,
       [this](const std::shared_ptr<ChatClient> &conn) { p2pRecvMsg(conn); }});

  msgHandler_.insert({MsgServiceId::MSG_CREATE_GROUP_ACK,
                      [this](const std::shared_ptr<ChatClient> &conn) {
                        createGroupACK(conn);
                      }});
}

void ChatService::regACK(const std::shared_ptr<ChatClient> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->read_message_.data());

  RegisterACK regACK;
  auto parseRet = regACK.ParseFromArray(conn->read_message_.data() +
                                            ChatProtocol::GetHeaderLength(),
                                        header->bodyLen_);
  if (parseRet) {
    fmt::print("regACK.regresult() = {}, regACK.reason() = {}\n",
               regACK.regresult(), regACK.reason());
  } else {
    fmt::print("failed to parse RegisterACK\n");
    conn->close();
  }
}

void ChatService::loginACK(const std::shared_ptr<ChatClient> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->read_message_.data());

  LoginACK loginACK;
  auto parseRet = loginACK.ParseFromArray(conn->read_message_.data() +
                                              ChatProtocol::GetHeaderLength(),
                                          header->bodyLen_);
  if (parseRet) {
    fmt::print("loginACK.loginresult() = {}, loginACK.reason() = {}, offline "
               "msg's count = {}, loginACK.friendlists_size() = {}\n",
               loginACK.loginresult(), loginACK.reason(),
               loginACK.offlinemsgs_size(), loginACK.friendlists_size());
    for (int i = 0; i < loginACK.offlinemsgs_size(); ++i) {
      fmt::print("recv offline's msg: {}\n", loginACK.offlinemsgs(i));
    }

    fmt::print("friend Lists:\n");
    for (int i = 0; i < loginACK.friendlists_size(); ++i) {
      auto &friendInfo = loginACK.friendlists(i);
      fmt::print("uid:{}, name:{}, status:{}\n", friendInfo.uid(),
                 friendInfo.name(), friendInfo.status());
    }
  } else {
    fmt::print("failed to parse LoginACK\n");
    conn->close();
  }
}

void ChatService::logoutACK(const std::shared_ptr<ChatClient> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->read_message_.data());

  LogoutACK logoutACK;
  auto parseRet = logoutACK.ParseFromArray(conn->read_message_.data() +
                                               ChatProtocol::GetHeaderLength(),
                                           header->bodyLen_);
  if (parseRet) {
    fmt::print("logoutACK.loginresult() = {}, logoutACK.reason() = {}\n",
               logoutACK.logoutresult(), logoutACK.reason());
  } else {
    fmt::print("failed to parse LogoutACK\n");
    conn->close();
  }
}

void ChatService::addFriendACK(const std::shared_ptr<ChatClient> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->read_message_.data());
  AddFriendACK addFriendACK;
  auto parseRet = addFriendACK.ParseFromArray(
      conn->read_message_.data() + ChatProtocol::GetHeaderLength(),
      header->bodyLen_);
  if (parseRet) {
    fmt::print("addFriendACK.loginresult() = {}, addFriendACK.reason() = {}\n",
               addFriendACK.addfriendresult(), addFriendACK.reason());
  } else {
    fmt::print("failed to parse LogoutACK\n");
    conn->close();
  }
}

void ChatService::handleAddFriendRes(const std::shared_ptr<ChatClient> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->read_message_.data());
  HandleFriendReq friendReq;
  auto parseRet = friendReq.ParseFromArray(conn->read_message_.data() +
                                               ChatProtocol::GetHeaderLength(),
                                           header->bodyLen_);
  if (parseRet) {
    // 维护状态
    fmt::print("handle frome {}'s friend request, addition = {}\n",
               friendReq.fromid(), friendReq.addition());
    // 这里需要ui交互然后保存状态, 后续由client主动发包给server
  } else {
    fmt::print("failed to parse LogoutACK\n");
    conn->close();
  }
}

void ChatService::p2pSendMsgACK(const std::shared_ptr<ChatClient> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->read_message_.data());
  P2PChatMsgACK chatACK;
  auto parseRet = chatACK.ParseFromArray(conn->read_message_.data() +
                                             ChatProtocol::GetHeaderLength(),
                                         header->bodyLen_);
  if (parseRet) {
    fmt::print("chatACK.msgstatus() = {}, chatACK.reason() = {}\n",
               chatACK.msgstatus(), chatACK.reason());
  } else {
    fmt::print("failed to parse P2PChatMsgACK\n");
    conn->close();
  }
}

void ChatService::heartbeatACK(const std::shared_ptr<ChatClient> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->read_message_.data());
  HeartbeatACK heartbeatACK;
  auto parseRet = heartbeatACK.ParseFromArray(
      conn->read_message_.data() + ChatProtocol::GetHeaderLength(),
      header->bodyLen_);
  if (parseRet) {
    fmt::print("heartbeatACK.random() = {}\n", heartbeatACK.random());
  } else {
    fmt::print("failed to parse LogoutACK\n");
    conn->close();
  }
}

void ChatService::createGroupACK(const std::shared_ptr<ChatClient> conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->read_message_.data());
  CreateGroupACK ack;
  auto parseRet = ack.ParseFromArray(conn->read_message_.data() +
                                         ChatProtocol::GetHeaderLength(),
                                     header->bodyLen_);
  if (parseRet) {
    fmt::print("群号={}\n", ack.groupid());
  } else {
    fmt::print("failed to parse CreateGroupACK\n");
    conn->close();
  }
}

void ChatService::p2pRecvMsg(const std::shared_ptr<ChatClient> &conn) {
  const ChatProtocol *header =
      reinterpret_cast<const ChatProtocol *>(conn->read_message_.data());
  P2PChatMsg chatMsg;
  auto parseRet = chatMsg.ParseFromArray(conn->read_message_.data() +
                                             ChatProtocol::GetHeaderLength(),
                                         header->bodyLen_);
  if (parseRet) {
    fmt::print("self[{}] recv [{}]'s message:{}\n", chatMsg.toid(),
               chatMsg.fromid(), chatMsg.msg());

    // ack to server
    P2PChatMsgACK msgACK;
    msgACK.set_msgstatus(0);
    msgACK.set_reason("成功接收消息");
    auto msg = ChatProtocol::genPacket(
        MsgServiceId::P2PMSG_SERVER_TO_CLIENT_ACK, msgACK.SerializeAsString());
    assert(msg.size() > ChatProtocol::GetHeaderLength());
    conn->write(std::move(msg));
  } else {
    fmt::print("failed to parse p2pRecvMsg\n");
    conn->close();
  }
}
