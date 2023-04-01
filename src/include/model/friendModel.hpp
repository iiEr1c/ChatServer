#pragma once

#include "userModel.hpp"

#include <vector>

struct FriendModel {
  bool insert(int userid, int friendid);

  std::vector<User> query(int userid);
};