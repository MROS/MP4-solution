#ifndef MATCH_QUEUE_HPP
#define MATCH_QUEUE

#include <deque>
#include <map>
#include "mq.hpp"
#include "user.hpp"
#include "client.hpp"
#include "linked_list.hpp"

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
  
  MQPair mq_pair;
  
  MatchQueue(MQPair mq_pair, std::map<int, Client*>* clients);
  
  void handle_child_crash();
  void handle_quit(int id);
  void handle_match(int id);
  void handle_report(ReportJob report);
  void add(int id);
  void arrange_job();
};

#endif
