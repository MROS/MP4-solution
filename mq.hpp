#ifndef MQ_HPP
#define MQ_HPP

#include <mqueue.h>
#include "user.hpp"

// 傳給 worker 的格式爲： 

struct AssignJob {
  struct User trying_user, candidate_user;
  int trying_id, candidate_id;
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