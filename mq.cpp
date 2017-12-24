#include "mq.hpp"
#include "match_queue.hpp"

#include <string>

using namespace std;

// 建立兩個消息隊列
// 1. 主程序用以向工人程序發送工作
// 2. 工人程序向主程序回報成果
MQPair create_MQ_pair() {
  
  printf("開始嘗試建立消息隊列\n");
  struct MQPair ret;
  
  struct mq_attr attr;
  
  attr.mq_flags = 0;  
  attr.mq_maxmsg = 10;  
  attr.mq_msgsize = sizeof(AssignJob);
  attr.mq_curmsgs = 0; 
  
  string prefix = "/inf-bonbon-server-from-main";
  int count = 0;
  while (true) {
    string name = prefix + to_string(count);
    mq_unlink(name.c_str());
    mqd_t mqd = mq_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
    if (mqd != -1) {
      ret.from_main_mqd = mqd;
      printf("建立消息隊列。名稱： %s mqd： %d\n", name.c_str(), mqd);
      break;
    } else {
      perror("建立消息隊列");
    }
    count += 1;
  }

  count = 0;
  prefix = "/inf-bonbon-server-from-worker";
  attr.mq_msgsize = sizeof(ReportJob);
  while (true) {
    string name = prefix + to_string(count);
    mq_unlink(name.c_str());
    mqd_t mqd = mq_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
    if (mqd != -1) {
      ret.from_worker_mqd = mqd;
      printf("建立消息隊列。名稱： %s mqd： %d\n", name.c_str(), mqd);
      break;
    } else {
      perror("建立消息隊列");
    }
    count += 1;
  }
  return ret;
}