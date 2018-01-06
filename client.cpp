#include "client.hpp"
#include "json.hpp"
#include "load_library_util.hpp"

#include <string>
#include <sstream>
#include <queue>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>

using namespace std;
using json = nlohmann::json;

User Client::to_user() {
  User user;
  user.age = this->raw_user.age;
  strcpy(user.name, this->raw_user.name);
  strcpy(user.gender, this->raw_user.gender);
  strcpy(user.introduction, this->raw_user.introduction);
  return user;
}


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

void Client::try_match_ack(json &try_match_json) {
  
  this->status = MATCHING;
  this->try_match_counter += 1;
  
  this->raw_user.age = try_match_json["age"].get<int>();
  strcpy(this->raw_user.name, try_match_json["name"].get<string>().c_str());
  strcpy(this->raw_user.gender, try_match_json["gender"].get<string>().c_str());
  strcpy(this->raw_user.introduction, try_match_json["introduction"].get<string>().c_str());
  strcpy(this->raw_user.filter_function, try_match_json["filter_function"].get<string>().c_str());
  
  save_source_and_compile(this->raw_user.filter_function, this->unique_id, this->try_match_counter);
  
  const char *msg = "{\"cmd\":\"try_match\"}\n";
  int err = send(this->socket_fd, msg, strlen(msg), 0);
  if (err == -1) {
    perror("send try_match_ack");
  }
  return;
}

void Client::quit() {
  
  this->status = IDLE;
  
  const char *msg = "{\"cmd\":\"quit\"}\n";
  int err = send(this->socket_fd, msg, strlen(msg), 0);
  if (err == -1) {
    perror("send quit_ack");
  }
  
  return;
}

void Client::other_side_quit() {
  this->status = IDLE;
  
  const char *msg = "{\"cmd\":\"other_side_quit\"}\n";
  int err = send(this->socket_fd, msg, strlen(msg), 0);
  if (err == -1) {
    perror("send quit_ack");
  }
  
  return;

}


void Client::send_message(json j) {
  // ack
  string request = j.dump() + "\n";
  int err = send(this->socket_fd, request.c_str(), strlen(request.c_str()), 0);
  if (err == -1) {
    perror("send send_message");
  }
  
  // to otherside
  j["cmd"] = "receive_message";
  request = j.dump() + "\n";
  err = send(this->match_fd, request.c_str(), strlen(request.c_str()), 0);
  if (err == -1) {
    perror("send receieve_message");
  }
  return;
}

string createMatchedAPI(Client *target) {
  json j;
  j["cmd"] = "matched";
  j["name"] = target->raw_user.name;
  j["age"] = target->raw_user.age;
  j["gender"] = target->raw_user.gender;
  j["introduction"] = target->raw_user.introduction;
  j["filter_function"] = target->raw_user.filter_function;
  cout << "matched API: " << j.dump() << endl;
  return j.dump();
}

void Client::handle_match(Client *target) {
  this->match_fd = target->socket_fd;
  target->match_fd = this->socket_fd;
  
  this->status = TALKING;
  target->status = TALKING;
  
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
