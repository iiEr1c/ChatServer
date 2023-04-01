#include <array>
#include <iostream>

struct User {
  std::array<char, 64> id;
  std::array<char, 64> password;
};