#pragma once

#include <string>

struct User {
  int id = -1;
  std::string name;
  std::string password;
  std::string state;
};

struct UserModel {
  bool insert(User &user);
  User query(int id);
  bool updateState(const User &user); // online or offline
};