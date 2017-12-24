#include "match_queue.hpp"
#include "mq.hpp"
#include "client.hpp"
#include "config.hpp"
#include "linked_list.hpp"

#include <deque>
#include <unistd.h>

using namespace std;

void work(MQPair mq_pair) {
  
}

MatchQueue::MatchQueue(MQPair mq_pair, map<int, Client*>* clients) {
  this->clients = clients;
  this->mq_pair = mq_pair;
  this->working_job = 0;
  mq_attr mqAttr;  
  mq_getattr(mq_pair.from_main_mqd, &mqAttr); 
  
  for (int i = 0; i < WORKER_NUMBER; i++) {
    pid_t pid = fork();
    if (pid == 0) { continue; }
    else {
      while (true) {
	AssignJob assign_job;
	int err = mq_receive(mq_pair.from_main_mqd, (char*)&assign_job, sizeof(AssignJob), NULL);
	if (err == -1) {
	  perror("mq_receive");
	}

	// TODO: 工作
	printf("企圖匹配 %d %d", assign_job.trying_id, assign_job.candidate_id);
	ReportJob report;
	report.result = true;
	report.trying_id = assign_job.trying_id;
	
	err = mq_send(mq_pair.from_worker_mqd, (char*)&report, sizeof(ReportJob), 0);
	if (err == -1) {
	  perror("mq_send");
	}
      }
    }
  }
}

void MatchQueue::handle_child_crash() {
  pid_t pid = fork();
  if (pid != 0) {
    
  }
}

void MatchQueue::handle_quit(int id) {
  Node<TryingUser> *trying_node = this->trying_queue.search([id](TryingUser user) { return user.id == id; });
  
  if (trying_node != nullptr) {
    this->trying_queue.erase(trying_node);
    return;
  }
  
  Node<int> *candidate_node = this->candidate_queue.search([id](int candidate_id) { return candidate_id == id; });
  
  if (candidate_node != nullptr) {
    this->candidate_queue.erase(candidate_node);
  }
  
}

void MatchQueue::handle_match(int id) {
 
  Node<TryingUser> *node = this->trying_queue.search([id](TryingUser user) { return user.id == id; });
 

  if (node != nullptr) {
 
    TryingUser trying_user = node->value;

    this->trying_queue.erase(node);
    this->candidate_queue.erase(trying_user.matchingWith);
 
    Client *client = (this->clients->operator[](trying_user.id));

    // handle_match 會傳送 matched 到雙方
    int candidate_id = trying_user.matchingWith->value;
    client->handle_match(this->clients->operator[](candidate_id));
    
  } else if (node == nullptr) {
  // TODO: 處理 match 時，已經 quit 的情形
    printf("處理 match 時，已經 quit\n");
  }
}

void MatchQueue::add(int id) {
  
  // candidate_quque 沒人
  if (this->candidate_queue.head == nullptr) {
    this->candidate_queue.push_back(id);
  }
  else {
    TryingUser trying_user;
    trying_user.id = id;
    trying_user.progress = WAITING;
    trying_user.matchingWith = this->candidate_queue.head;
    this->trying_queue.push_back(trying_user);
    this->arrange_job();
  }
  
}

void MatchQueue::arrange_job() {
  Node<TryingUser> *iter = this->trying_queue.head;
  
  // TODO: 改爲多人版
  while (iter != nullptr) {
    if (iter->value.progress == WAITING) {
      iter->value.progress = PROCESSING;
      
      AssignJob assign_job;
      assign_job.trying_id = iter->value.id;
      assign_job.candidate_id = iter->value.matchingWith->value;
      assign_job.trying_user = this->clients->operator[](assign_job.trying_id)->to_user();
      assign_job.candidate_user = this->clients->operator[](assign_job.candidate_id)->to_user();
      
      mq_send(this->mq_pair.from_main_mqd, (char *)&assign_job, sizeof(assign_job), 0);
      break;
    }
    iter = iter->next;
  }
}

void MatchQueue::handle_report(ReportJob report) {
  // TODO: 處理已經 quit 的狀況
  if (report.result == true) {
    this->handle_match(report.trying_id);
    this->arrange_job();
  } else if (report.result == false) {
    
    int id = report.trying_id;
    Node<TryingUser> *node = this->trying_queue.search([id](TryingUser user) { return user.id == id; });
    
    if (node->value.matchingWith == this->candidate_queue.tail) {
      this->candidate_queue.push_back(id);
      this->trying_queue.erase(node);
    } else {
      node->value.progress = WAITING;
    }
    
    this->arrange_job();
  }
}











