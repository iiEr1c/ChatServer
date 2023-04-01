#include "model/groupModel.hpp"
#include "common/db/db.hpp"

#include <fmt/core.h>

#include <cassert>

bool GroupModel::createGroup(Group &group) {
  auto sql = fmt::format(
      "insert into allGroups(groupname, groupdesc) values('{}', '{}')",
      group.name, group.desc);
  IM::util::MySQL mysql;
  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json") &&
      mysql.update(sql)) {
    group.id = mysql_insert_id(mysql.getConnection());
    return true;
  } else {
    fmt::print("failed to execute {}\n", sql);
    return false;
  }
}

bool GroupModel::addGroup(int userid, uint64_t groupid, GroupUser::Role role) {
  auto sql = fmt::format(
      "insert into groupMembers(groupid, userid, role) values({}, {}, {})",
      groupid, userid, static_cast<int>(role));
  IM::util::MySQL mysql;
  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json") &&
      mysql.update(sql)) {
    return true;
  } else {
    fmt::print("failed to execute {}\n", sql);
    return false;
  }
}

bool GroupModel::exitGroup(int userid, uint64_t groupid) {
  auto sql =
      fmt::format("delete from groupMembers where groupid = {} and userid = {}",
                  groupid, userid);
  IM::util::MySQL mysql;
  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json") &&
      mysql.update(sql)) {
    return true;
  } else {
    fmt::print("failed to execute {}\n", sql);
    return false;
  }
}

std::vector<Group> GroupModel::queryGroup(int userid) {
  auto sql = fmt::format(
      "select a.id, a.groupname, a.groupdesc from allGroups a inner join "
      "groupMembers b on a.id = b.groupid where b.userid = {}",
      userid);
  IM::util::MySQL mysql;
  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json")) {
    std::vector<Group> result;
    MYSQL_RES *res = mysql.query(sql);
    if (res != nullptr) {
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(res)) != nullptr) {
        assert(std::atoi(row[0]) > 0);
        Group group{.id = static_cast<uint64_t>(std::atoi(row[0])),
                    .name = std::string(row[1]),
                    .desc = std::string(row[2])};
        result.emplace_back(std::move(group));
      }
      mysql_free_result(res);
    }
    return result;
  } else {
    fmt::print("failed to execute {}\n", sql);
    return {};
  }
}

std::vector<int> GroupModel::queryGroupMember(uint64_t groupid, int userid) {
  auto sql = fmt::format(
      "select userid from groupMembers where groupid = {} and userid != {}",
      groupid, userid);

  IM::util::MySQL mysql;
  std::vector<int> otherMembers;

  if (mysql.connect("/home/er1c/code/IM/conf/mysql_conf.json")) {
    MYSQL_RES *res = mysql.query(sql);
    if (res != nullptr) {
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(res)) != nullptr) {
        otherMembers.push_back(std::atoi(row[0]));
      }
    }
    return otherMembers;
  } else {
    fmt::print("failed to execute {}\n", sql);
    return {};
  }
}