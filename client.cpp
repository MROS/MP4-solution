#include "client.hpp"
#include "json.hpp"

#include <string>
#include <sstream>
#include <queue>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>

using namespace std;
using json = nlohmann::json;

int count_char(char *s, char c) {
  int len = strlen(s);
  int count = 0;
  for (int i = 0; i < len; i++) {
    if (s[i] == c) {
      count += 1;
    }
  }
  return count;
}

void Client::socket_recv(char *s) {
  int cmd_number = count_char(s, '\n');
  this->buffer << s;
  for (int i = 0; i < cmd_number; i++) {
    string str;
    std::getline(this->buffer, str, '\n');
    this->cmd_queue.push(str);
  }
  return;
}

void Client::try_match_ack() {
  const char *msg = "{\"cmd\":\"try_match\"}\n";
  int err = send(this->socket_fd, msg, strlen(msg), 0);
  if (err == -1) {
    perror("send try_match_ack");
  }
  return;
}

void Client::quit_ack() {
  const char *msg = "{\"cmd\":\"quit\"}\n";
  int err = send(this->socket_fd, msg, strlen(msg), 0);
  if (err == -1) {
    perror("send quit_ack");
  }
  return;
}

void Client::send_message_ack(string request) {
  request += "\n";
  int err = send(this->socket_fd, request.c_str(), strlen(request.c_str()), 0);
  if (err == -1) {
    perror("send send_message_ack");
  }
  return;
}

string createMatchedAPI(Client *target) {
  json j;
  j["cmd"] = "matched";
  j["name"] = target->user.name;
  // TODO: 待補其他資訊
  return j.dump();
}

void Client::handle_match(Client *target) {
  this->match_fd = target->socket_fd;
  target->match_fd = this->socket_fd;
  
  string api = createMatchedAPI(target) + "\n";
  int err = send(this->socket_fd, api.c_str(), api.size(), 0);
  if (err == -1) {
    perror("send matched");
  }
  
  api = createMatchedAPI(this) + "\n";
  err = send(target->socket_fd, api.c_str(), api.size(), 0);
  if (err == -1) {
    perror("send matched");
  }
  
  return;
}
