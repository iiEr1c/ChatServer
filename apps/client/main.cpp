#include "GroupChat.pb.h"
#include "GroupChatACK.pb.h"
#include "Heartbeat.pb.h"
#include "HeartbeatACK.pb.h"
#include "Login.pb.h"
#include "LoginACK.pb.h"
#include "Logout.pb.h"
#include "LogoutACK.pb.h"
#include "P2PChatMsg.pb.h"
#include "Register.pb.h"
#include "RegisterACK.pb.h"

#include "ChatClient.hpp"
#include "CreateGroup.pb.h"

#include <thread>

void test_reg(const std::shared_ptr<ChatClient> &client, std::string username,
              std::string password) {
  fmt::print("test_reg(const std::shared_ptr<ChatClient> &)\n");
  Register userInfo;
  userInfo.set_name(username);
  userInfo.set_password(password);
  std::string serialUserInfo;
  auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_REG,
                                     userInfo.SerializeAsString());
  assert(msg.size() > ChatProtocol::GetHeaderLength());
  client->write(std::move(msg));
}

void test_login(const std::shared_ptr<ChatClient> &client, int32_t userid,
                std::string password) {
  fmt::print("test_login(const std::shared_ptr<ChatClient> &)\n");
  Login loginInfo;
  loginInfo.set_id(userid);
  loginInfo.set_password(password);
  auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_LOGIN,
                                     loginInfo.SerializeAsString());
  assert(msg.size() > ChatProtocol::GetHeaderLength());
  client->write(std::move(msg));
}

void test_logout(const std::shared_ptr<ChatClient> &client, int32_t userid) {
  fmt::print("test_logout(const std::shared_ptr<ChatClient> &)\n");
  Logout logoutInfo;
  logoutInfo.set_id(userid);
  auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_LOGOUT,
                                     logoutInfo.SerializeAsString());
  assert(msg.size() > ChatProtocol::GetHeaderLength());
  client->write(std::move(msg));
}

void test_login_then_logout(const std::shared_ptr<ChatClient> &client,
                            int32_t userid, std::string password) {
  fmt::print("test_login_then_logout(const std::shared_ptr<ChatClient> &)\n");
  Login loginInfo;
  loginInfo.set_id(userid);
  loginInfo.set_password(password);
  auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_LOGIN,
                                     loginInfo.SerializeAsString());
  assert(msg.size() > ChatProtocol::GetHeaderLength());
  client->write(std::move(msg));

  // 处理返回的消息

  fmt::print("--------------------------------------------\n");
  // logout, todo: 密码校验?
  Logout logoutInfo;
  logoutInfo.set_id(22);
  auto logoutMsg = ChatProtocol::genPacket(MsgServiceId::MSG_LOGOUT,
                                           logoutInfo.SerializeAsString());
  assert(logoutMsg.size() > ChatProtocol::GetHeaderLength());
  client->write(std::move(logoutMsg));
}

void test_add_friend(const std::shared_ptr<ChatClient> &client) {}

void test_heartbeat(const std::shared_ptr<ChatClient> &client) {
  fmt::print("test_heartbeat(const std::shared_ptr<ChatClient> &)\n");
  Heartbeat heartbeat;
  heartbeat.set_random(1);
  auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_HEARTBEAT,
                                     heartbeat.SerializeAsString());
  assert(msg.size() > ChatProtocol::GetHeaderLength());
  client->write(std::move(msg));
}

void test_offline_sendMsg(const std::shared_ptr<ChatClient> &client,
                          const std::shared_ptr<ChatClient> &client2, int from,
                          int to, std::string message) {
  fmt::print("test_offline_sendMsg(const std::shared_ptr<ChatClient> &)\n");
  P2PChatMsg chatMsg;
  chatMsg.set_fromid(from);
  chatMsg.set_toid(to);
  chatMsg.set_msg(message);
  auto msg = ChatProtocol::genPacket(MsgServiceId::P2PMSG_CLIENT_TO_SERVER,
                                     chatMsg.SerializeAsString());
  assert(msg.size() > ChatProtocol::GetHeaderLength());
  client->write(std::move(msg));

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1s);
  // client2登录, 此时应该收到离线消息
  fmt::print("client2登录, 验证离线消息.................\n");
  test_login(client2, to, "password");
  test_logout(client2, to);
}

void test_same_server_sendMsg(const std::shared_ptr<ChatClient> &client,
                              const std::shared_ptr<ChatClient> &client2,
                              int from, int to, std::string message) {
  fmt::print("test_same_server_sendMsg(const std::shared_ptr<ChatClient> &)\n");

  // 22 和 28登录
  test_login(client, from, "password");
  test_login(client2, to, "password");

  P2PChatMsg chatMsg;
  chatMsg.set_fromid(from);
  chatMsg.set_toid(to);
  chatMsg.set_msg(message);
  auto msg = ChatProtocol::genPacket(MsgServiceId::P2PMSG_CLIENT_TO_SERVER,
                                     chatMsg.SerializeAsString());
  assert(msg.size() > ChatProtocol::GetHeaderLength());
  client->write(std::move(msg));

  test_logout(client, from);
  test_logout(client2, to);
}

void test_groupchat(const std::shared_ptr<ChatClient> &client, int userid,
                    int groupid, std::string message) {

  fmt::print("test_groupchat(const std::shared_ptr<ChatClient> &)\n");
  GroupChat groupChat;
  groupChat.set_fromid(userid);
  groupChat.set_toid(groupid);
  groupChat.set_msg(message);
  auto msg = ChatProtocol::genPacket(MsgServiceId::GROUPMSG_CLIENT_TO_SERVER,
                                     groupChat.SerializeAsString());
  assert(msg.size() > ChatProtocol::GetHeaderLength());
  client->write(std::move(msg));
}
/*
  22 hello password
*/

void test_createGroup(const std::shared_ptr<ChatClient> &client, int creator,
                      std::string groupName, std::string groupDesc) {
  fmt::print("test_createGroup(const std::shared_ptr<ChatClient> &)\n");
  CreateGroup createGroupInfo;
  createGroupInfo.set_creator(creator);
  createGroupInfo.set_groupname(groupName);
  createGroupInfo.set_groupdesc(groupDesc);

  auto msg = ChatProtocol::genPacket(MsgServiceId::MSG_CREATE_GROUP,
                                     createGroupInfo.SerializeAsString());
  assert(msg.size() > ChatProtocol::GetHeaderLength());
  client->write(std::move(msg));
}

void test() {
  boost::asio::io_context io_context;
  tcp::resolver resolver(io_context);
  auto endpoints = resolver.resolve("127.0.0.1", "8000");
  auto client = std::make_shared<ChatClient>(io_context, endpoints);
  auto client2 = std::make_shared<ChatClient>(io_context, endpoints);
  assert(client != nullptr && client2 != nullptr);

  // test_reg(client, "john", "password");

  // test_login_then_logout(client, 22, "password"); // 密码正确

  // test_login(client, 22, "uncorrect password"); // 错误密码

  // test_heartbeat(client);

  // test_offline_sendMsg(client, client2, 22, 28, "first message");

  // test_same_server_sendMsg(client, client2, 22, 28, "from[22] to[28]");

  // test_groupchat(client, 22, 1, "first group message");
  test_createGroup(client, 22, "第一个群", "群简介");

  auto thr = std::thread([&]() { io_context.run(); });
  using namespace std::literals::chrono_literals;
  std::this_thread::sleep_for(15s);

  client->close();
  client2->close();

  thr.join();
}

int main() {
  test();
  return 0;
}