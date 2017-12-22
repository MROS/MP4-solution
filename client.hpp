#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <queue>
#include <sstream>
#include <string>

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
  RawUser user;
  int match_fd;
  int socket_fd;
  
  int unique_id;
  
  std::queue<std::string> cmd_queue;

  Client(int fd, int id) : socket_fd(fd), unique_id((id)), match_fd(-1) {}
  void socket_recv(char *s);
  void send_message_ack(std::string request);
  void quit_ack();
  void try_match_ack();
  void handle_match(Client *target);
};

#endif