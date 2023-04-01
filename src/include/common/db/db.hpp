#pragma once

#include <mysql/mysql.h>

#include <string>

namespace IM {
namespace util {
class MySQL {
public:
  MySQL();
  ~MySQL();
  bool connect(const std::string &confPath);
  bool update(const std::string &sql);
  MYSQL_RES *query(const std::string &sql);
  MYSQL *getConnection();

  MySQL(const MySQL &) = delete;
  MySQL &operator=(const MySQL &) = delete;

private:
  MYSQL m_conn;
};
}; // namespace util
}; // namespace IM