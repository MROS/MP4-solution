#ifndef MQ_HPP
#define MQ_HPP

#include <mqueue.h>
#include <unistd.h>
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

struct ReportPid {
  pid_t pid;
  int trying_id, candidate_id;
};

struct ReportJob {
  pid_t pid;
  bool result;
  int trying_id, candidate_id;
};

struct MQSet {
   mqd_t from_main_mqd, from_worker_job_mqd, from_worker_pid_mqd;
};

MQSet create_MQ_pair();


#endif