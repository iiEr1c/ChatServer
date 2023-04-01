#include "common/db/db.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("MySQL::connect", "MySQL::connect") {
  IM::util::MySQL mysql;
  REQUIRE(true == mysql.connect("/home/er1c/code/IM/tests/mysql_conf.json"));
}

TEST_CASE("MySQL::update", "MySQL::update") {
  IM::util::MySQL mysql;
  REQUIRE(true == mysql.connect("/home/er1c/code/IM/tests/mysql_conf.json"));
  // REQUIRE(true == mysql.update("insert into a(id) values(1)"));
}

TEST_CASE("MySQL::query", "MySQL::query") {
  IM::util::MySQL mysql;
  REQUIRE(true == mysql.connect("/home/er1c/code/IM/tests/mysql_conf.json"));
  MYSQL_RES *res = mysql.query("select id from a");
  if (res == nullptr) {
    return;
  }
  int num_fields = ::mysql_num_fields(res);
  MYSQL_ROW row;
  while ((row = ::mysql_fetch_row(res))) {
    for (int i = 0; i < num_fields; ++i) {
      printf("%s\n", row[i]);
    }
  }
  ::mysql_free_result(res);
}