#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <queue>
#include <sstream>
#include <string>
using namespace std;

struct User {
  char name[33];
  unsigned int age;
  char gender[7];
  char introduction[1025];
  char filter_function[4097];
};

class Client {
public:
  stringstream buffer;
  User user;
  int match_fd;
  int socket_fd;
  queue<string> cmd_queue;

  Client(int fd) : socket_fd(fd), match_fd(-1) {}
  void recv_message(char *s);
};

#endif