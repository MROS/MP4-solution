#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <queue>
#include <sstream>
#include <string>
#include "user.hpp"
#include "json.hpp"

enum ClientStatus {
  IDLE,
  MATCHING,
  TALKING,
};

struct RawUser {
  char name[33];
  unsigned int age;
  char gender[7];
  char introduction[1025];
  char filter_function[4097];
};

class Client {
public:
  std::stringstream buffer;
  unsigned int try_match_counter;  // 記錄是第幾次 try_match ，好讓子進程分辨在篩選函式編譯到哪一個動態函式庫
  ClientStatus status;
  RawUser raw_user;
  int match_fd;
  int socket_fd;
  
  int unique_id;
  
  std::queue<std::string> cmd_queue;

  Client(int fd, int id) : socket_fd(fd), unique_id((id)),
  match_fd(-1), status(IDLE), try_match_counter(0) {}
  
  struct User to_user();
  void socket_recv(char *s);
  void send_message(nlohmann::json j);
  void quit();
  void other_side_quit();
  void try_match_ack(nlohmann::json &try_match_json);
  void handle_match(Client *target);
};

#endif