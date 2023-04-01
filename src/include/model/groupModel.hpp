#pragma once

#include "userModel.hpp"

#include <string>
#include <vector>

struct GroupUser : public User {
  enum Role { HOST = 0, ADMIN, MEMBER };
  int role; // 群主/管理员/普通成员
};

struct Group {
  uint64_t id;                        // 群id
  std::string name;                   // 群名字
  std::string desc;                   // 群描述
  std::vector<GroupUser> memberLists; // 群所有成员
};

struct GroupModel {
  bool createGroup(Group &);
  bool addGroup(int userid, uint64_t groupid, GroupUser::Role role);
  bool exitGroup(int userid, uint64_t groupid);
  std::vector<Group> queryGroup(int userid); // 查询用户在的所有群
  std::vector<int> queryGroupMember(uint64_t groupid, int userid); // 查询群内的所有成员
};