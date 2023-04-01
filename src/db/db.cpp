#include "common/db/db.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

#include <fmt/color.h>

using json = nlohmann::json;

namespace IM {
namespace util {
MySQL::MySQL() { ::mysql_init(&m_conn); }

MySQL::~MySQL() { ::mysql_close(&m_conn); }

bool MySQL::connect(const std::string &confPath) {
  std::ifstream confFile(confPath);
  if (confFile.is_open()) {
    json conf;
    confFile >> conf;
    ::MYSQL *conn =
        ::mysql_real_connect(&m_conn, conf["ip"].get<std::string>().c_str(),
                             conf["user"].get<std::string>().c_str(),
                             conf["password"].get<std::string>().c_str(),
                             conf["dbname"].get<std::string>().c_str(),
                             conf["port"].get<unsigned int>(), nullptr, 0);
    if (conn != nullptr) {
      ::mysql_query(&m_conn, "set names utf8");
      fmt::print("connect to mysql: {}:{}, user:{}, dbname:{}\n",
               conf["ip"].get<std::string>(), conf["port"].get<unsigned int>(),
               conf["user"].get<std::string>(),
               conf["dbname"].get<std::string>());
      return true;
    } else {
      fmt::print("failed to connect to mysql: {}:{}, user:{}, dbname:{}\n",
               conf["ip"].get<std::string>(), conf["port"].get<unsigned int>(),
               conf["user"].get<std::string>(),
               conf["dbname"].get<std::string>());
      return false;
    }
  } else {
    return false;
  }
}

bool MySQL::update(const std::string &sql) {

  if (::mysql_query(&m_conn, sql.c_str())) {
    fmt::print("failed to execute update statement '{}'\n", sql);
    return false;
  }
  return true;
}

MYSQL_RES *MySQL::query(const std::string &sql) {
  if (::mysql_query(&m_conn, sql.c_str())) {
    fmt::print("failed to execute query statement '{}'\n", sql);
    return nullptr;
  }
  return mysql_use_result(&m_conn);
}

MYSQL *MySQL::getConnection() { return &m_conn; }

} // namespace util
} // namespace IM