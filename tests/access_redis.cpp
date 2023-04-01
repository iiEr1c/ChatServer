#include "common/redis/redis.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Redis::connect", "Redis::connect") {
  auto redis = std::make_shared<IM::util::Redis>();
  REQUIRE(true == redis->connect("/home/er1c/code/IM/tests/redis_conf.json"));
}

TEST_CASE("Redis::subscribe", "Redis::subscribe") {
  auto redis = std::make_shared<IM::util::Redis>();
  REQUIRE(true == redis->connect("/home/er1c/code/IM/tests/redis_conf.json"));
  REQUIRE(true == redis->subscribe(1));
  sleep(15);
}