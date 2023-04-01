#pragma once

#include <string>
#include <vector>

struct OfflineMsgModel {
  bool insert(int from, int to, const std::string &msg);
  bool remove(int userid);
  std::vector<std::string> query(int userid);
};