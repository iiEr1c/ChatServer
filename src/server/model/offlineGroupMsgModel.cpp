#include "model/offlineGroupMsgModel.hpp"
#include "common/db/db.hpp"

#include <fmt/core.h>

bool OfflineGroupMsgModel::insert(int from, int groupid, int to,
                                  const std::string &msg) {
  auto sql = fmt::format("insert into offlineGroupMsg(fromid, groupid, toid, "
                         "message) values({}, {}, {}, '{}')",
                         from, groupid, to, msg.c_str());
  IM::util::MySQL mysql;
  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json") &&
      mysql.update(sql)) {
    return true;
  }
  return false;
}

bool OfflineGroupMsgModel::remove(int userid) {
  auto sql = fmt::format("delete from offlineGroupMsg where toid = {}", userid);
  IM::util::MySQL mysql;
  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json") &&
      mysql.update(sql)) {
    return true;
  }
  return false;
}

std::vector<std::string> OfflineGroupMsgModel::query(int userid) {
  std::vector<std::string> result;
  auto sql = fmt::format("select message from offlineGroupMsg where toid = {}",
                         userid);
  IM::util::MySQL mysql;
  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json")) {
    MYSQL_RES *res = mysql.query(sql);
    if (res != nullptr) {
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(res)) != nullptr) {
        result.push_back(std::string(row[0]));
      }
      mysql_free_result(res);
      return result;
    }
  }
  return result;
}