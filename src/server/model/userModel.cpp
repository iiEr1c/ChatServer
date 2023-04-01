#include "model/userModel.hpp"
#include "common/db/db.hpp"

#include "fmt/core.h"

#include <cassert>

bool UserModel::insert(User &user) {
  auto sql = fmt::format(
      "insert into user(name, password, state) values('{}', '{}', '{}')",
      user.name.c_str(), user.password.c_str(), user.state.c_str());
  IM::util::MySQL mysql;
  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json") &&
      mysql.update(sql)) {
    user.id = mysql_insert_id(mysql.getConnection());
    return true;
  }
  return false;
}

User UserModel::query(int id) {
  auto sql = fmt::format(
      "select id, name, password, state from user where id = {}", id);
  IM::util::MySQL mysql;
  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json")) {
    MYSQL_RES *res = mysql.query(sql);
    if (res != nullptr) {
      if (MYSQL_ROW row = mysql_fetch_row(res); row != nullptr) {
        User user;
        user.id = ::atoi(row[0]);
        user.name = std::string(row[1]);
        user.password = std::string(row[2]);
        user.state = std::string(row[3]);
        mysql_free_result(res);
        return user;
      }
    }
  }
  return {};
}

bool UserModel::updateState(const User &user) {
  assert(user.id != -1);
  auto sql = fmt::format("update user set state = '{}' where id = {}",
                         user.state.c_str(), user.id);
  IM::util::MySQL mysql;
  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json") &&
      mysql.update(sql)) {
    return true;
  }
  return false;
}