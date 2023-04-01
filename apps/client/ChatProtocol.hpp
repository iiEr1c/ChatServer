#pragma once

#include <cstdint>
#include <cstring>

#include <string>

struct ChatProtocol {
  static inline uint32_t MaxBodyLength = 1024;

  constexpr static inline uint32_t GetHeaderLength() {
    return sizeof(magic_number_) + sizeof(version_) + sizeof(serviceId_) +
           sizeof(sequence_) + sizeof(bodyLen_);
  }

  static ChatProtocol createHeader(uint16_t serviceid, uint32_t bodyLen) {
    return ChatProtocol{.magic_number_ = static_cast<unsigned char>(0xAA),
                        .version_ = static_cast<unsigned char>(0x01),
                        .serviceId_ = static_cast<uint16_t>(serviceid),
                        .sequence_ = static_cast<uint32_t>(0x01),
                        .bodyLen_ = static_cast<uint32_t>(bodyLen)};
  }

  // assume data's size >= GetHeaderLength()
  static void createHeader(void *data, uint16_t serviceid, uint32_t bodyLen) {
    *reinterpret_cast<ChatProtocol *>(data) = {
        .magic_number_ = static_cast<unsigned char>(0xAA),
        .version_ = static_cast<unsigned char>(0x01),
        .serviceId_ = static_cast<uint16_t>(serviceid),
        .sequence_ = static_cast<uint32_t>(0x01),
        .bodyLen_ = static_cast<uint32_t>(bodyLen)};
  }

  static std::string genPacket(uint16_t serviceid, std::string body) {
    std::string msg;
    msg.resize(ChatProtocol::GetHeaderLength() + body.size());
    ChatProtocol::createHeader(msg.data(), serviceid, body.size());
    ::memcpy(msg.data() + ChatProtocol::GetHeaderLength(), body.data(),
             body.size());
    return msg;
  }

  bool isValid() const {
    return magic_number_ == static_cast<unsigned char>(0xAA) &&
           version_ == static_cast<unsigned char>(1) &&
           bodyLen_ <= MaxBodyLength && bodyLen_ > 0;
  }

  bool decodeHeader(const unsigned char *input) {
    // assume input's length == HeaderLength()
    ::memcpy(this, input, GetHeaderLength());
    if (magic_number_ != static_cast<unsigned char>(0xAA) ||
        version_ != static_cast<unsigned char>(1)) {
      return false;
    }
    return true;
  }

  void encodeHeader(unsigned char *output) {
    ::memcpy(output, this, GetHeaderLength());
  }

  unsigned char magic_number_; // 0xAA
  unsigned char version_;      // 0x1
  uint16_t serviceId_;         // 服务id
  uint32_t sequence_;          // 消息序号
  uint32_t bodyLen_;           // body长度
};

// map to serviceId
enum MsgServiceId {
  MSG_LOGIN = 0,
  MSG_LOGIN_ACK,
  MSG_LOGOUT,
  MSG_LOGOUT_ACK,
  MSG_REG,
  MSG_REG_ACK,
  MSG_ADD_FRIEND,
  MSG_ADD_FRIEND_ACK,
  MSG_ADD_FRIEND_REQ, // 区分MSG_ADD_FRIEND_ACK, 这里就是为了发给client然后判断他是否同意
  MSG_ADD_FRIEND_RES,
  MSG_HEARTBEAT,
  MSG_HEARTBEAT_ACK,
  P2PMSG_CLIENT_TO_SERVER,
  P2PMSG_CLIENT_TO_SERVER_ACK, // 发送方消息到达服务器, 服务器给发送方的回执
  P2PMSG_SERVER_TO_CLIENT,
  P2PMSG_SERVER_TO_CLIENT_ACK, // 服务器将消息转发客户端
  GROUPMSG_CLIENT_TO_SERVER,
  GROUPMSG_CLIENT_TO_SERVER_ACK,
  GROUPMSG_SERVER_TO_CLIENT,
  GROUPMSG_SERVER_TO_CLIENT_ACK,
  MSG_CREATE_GROUP,
  MSG_CREATE_GROUP_ACK
};