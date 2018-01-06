#ifndef MATCH_QUEUE_HPP
#define MATCH_QUEUE

#include <map>
#include <set>
#include "mq.hpp"
#include "user.hpp"
#include "client.hpp"
#include "linked_list.hpp"

void *get_library_handle(Info *info);

int run_filter(void *handle, User user);

enum Progress {
  WAITING, PROCESSING 
};

struct TryingUser {
  int id;
  Node<int> *matchingWith;
  Progress progress;
};

class MatchQueue {
public:
  LinkedList<int> candidate_queue;
  LinkedList<TryingUser> trying_queue;
  
  std::map<int, Client*>* clients;
  
  unsigned int working_job;
  
  MQSet mq_pair;
  
  MatchQueue(MQSet mq_pair, std::map<int, Client*>* clients);
  
  void handle_child_crash(pid_t child_pid);
  void handle_quit(int id);
  void handle_match(int id);
  void handle_report(ReportJob report);
  void handle_report_pid(ReportPid report_pid);
  void add(int id);
  void arrange_job();
private:
  std::map<pid_t, ReportPid> job_record;
  std::set<int> quit_set;
  void detach_with_matcher(Node<TryingUser> *trying_node);
  void detach(int id);
};

#endif
