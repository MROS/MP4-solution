#ifndef MQ_HPP
#define MQ_HPP

#include <mqueue.h>

struct MQPair {
   mqd_t from_main_mqd, from_worker_mqd;
};


#endif