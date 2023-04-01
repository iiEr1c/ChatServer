#include "model/friendModel.hpp"
#include "common/db/db.hpp"

#include <fmt/core.h>

#include <cassert>

bool FriendModel::insert(int userid, int friendid) {
  assert(userid > 0 && friendid > 0);
  auto sql = fmt::format(
      "insert into friends(userid, friendid) values ({}, {}),({}, {})", userid,
      friendid, friendid, userid);
  IM::util::MySQL mysql;
  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json") &&
      mysql.update(sql)) {
    return true;
  }
  return false;
}

std::vector<User> FriendModel::query(int userid) {
  assert(userid > 0);
  auto sql = fmt::format("select a.id, a.name, a.state from user a inner join "
                         "friends b on a.id = b.friendid where b.userid = {}",
                         userid);
  std::vector<User> friends;
  IM::util::MySQL mysql;
  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json")) {
    auto *res = mysql.query(sql);
    if (res != nullptr) {
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(res)) != nullptr) {
        friends.emplace_back(atoi(row[0]), row[1], "", row[2]);
      }

      // free res resource
      mysql_free_result(res);
    }
  }
  return friends;
}