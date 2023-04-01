#pragma once

#include <string>
#include <vector>

struct OfflineGroupMsgModel {
  bool insert(int from, int groupid, int to, const std::string &msg);
  bool remove(int userid);
  std::vector<std::string> query(int userid);
};