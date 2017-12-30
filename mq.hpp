#ifndef MQ_HPP
#define MQ_HPP

#include <mqueue.h>
#include "user.hpp"

struct Info {
  int id;
  struct User user;
  unsigned int counter;
};

// 傳給 worker 的格式爲：
struct AssignJob {
  Info trying_info, candidate_info;
};

struct ReportJob {
  bool result;
  int trying_id;
};

struct MQPair {
   mqd_t from_main_mqd, from_worker_mqd;
};

MQPair create_MQ_pair();


#endif