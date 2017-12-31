#include "match_queue.hpp"
#include "mq.hpp"
#include "client.hpp"
#include "config.hpp"
#include "linked_list.hpp"

#include <dlfcn.h>
#include <deque>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string>

using namespace std;


ReportJob do_job(AssignJob *assign_job) {
  ReportJob report;
  report.trying_id = assign_job->trying_info.id;
  
  void *handle = get_library_handle(&(assign_job->trying_info));
  int result = run_filter(handle, assign_job->candidate_info.user);
  dlclose(handle);
  if (result == 0) {
    printf("in do_job, trying filter_function false\n");
    report.result = false;
    return report;
  }

  handle = get_library_handle(&(assign_job->candidate_info));
  result = run_filter(handle, assign_job->trying_info.user);
  dlclose(handle);
  if (result == 0) {
    printf("in do_job, candidate filter_function false\n");
    report.result = false;
    return report;
  }

  report.result = true;
  return report;
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
	
	printf("試圖匹配 %d %d\n", assign_job.trying_info.id, assign_job.candidate_info.id);
	ReportJob report = do_job(&assign_job);
	
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

void MatchQueue::detach(Node<TryingUser>* trying_node) {
  
  Node<int>* candidate_node = trying_node->value.matchingWith;
  
  if (trying_node == nullptr || candidate_node == nullptr) {
    printf("試圖在隊伍中丟掉爲 nullptr 的節點\n");
    exit(1);
  }
  
  bool is_tail = (this->candidate_queue.length == 1);
  
  Node<TryingUser> *iter = trying_node->next;
  if (is_tail) {
    if (iter != nullptr) {
      this->candidate_queue.push_back(iter->value.id);
      this->trying_queue.erase(iter);
    }
  }

  Node<int> *next_candidate = candidate_node->next;
  iter = trying_node->next;          // 原本的 next 可能在上面被 erase 掉了
  while (iter != nullptr && iter->value.matchingWith == candidate_node) {
    iter->value.matchingWith = next_candidate;
    iter = iter->next;
  }
  
  this->trying_queue.erase(trying_node);
  this->candidate_queue.erase(candidate_node);

}

void MatchQueue::handle_quit(int id) {
  Node<TryingUser> *trying_node = this->trying_queue.search([id](TryingUser user) { return user.id == id; });
  this->detach(trying_node);
}

void MatchQueue::handle_match(int id) {
 
  Node<TryingUser> *trying_node = this->trying_queue.search([id](TryingUser user) { return user.id == id; });
 
  if (trying_node != nullptr) {
 
    TryingUser trying_user = trying_node->value;
    Client *client = (this->clients->operator[](trying_user.id));

    // handle_match 會傳送 matched 到雙方
    int candidate_id = trying_user.matchingWith->value;
    client->handle_match(this->clients->operator[](candidate_id));
    
    this->detach(trying_node);
    
  } else if (trying_node == nullptr) {
    // TODO: 處理 match 時，已經 quit 的情形
    // 測試時，保證不會發生。但現實世界會發生
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
    bool ok = (iter->value.progress == WAITING);
    if (iter->prev != nullptr) {
      ok = ok && (iter->prev->value.matchingWith != iter->value.matchingWith);
    }
    if (ok) {
      iter->value.progress = PROCESSING;
      
      int trying_id = iter->value.id;
      int candidate_id =  iter->value.matchingWith->value;
      AssignJob assign_job;
      assign_job.trying_info.id = trying_id;
      assign_job.candidate_info.id = candidate_id;
      assign_job.trying_info.user = this->clients->operator[](trying_id)->to_user();
      assign_job.candidate_info.user = this->clients->operator[](candidate_id)->to_user();
      assign_job.trying_info.counter = this->clients->operator[](trying_id)->try_match_counter;
      assign_job.candidate_info.counter = this->clients->operator[](candidate_id)->try_match_counter;
      
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
    
    printf("report 爲 false\n");
    
    int id = report.trying_id;
    Node<TryingUser> *node = this->trying_queue.search([id](TryingUser user) { return user.id == id; });
    
    if (node->value.matchingWith == this->candidate_queue.tail) {
      this->candidate_queue.push_back(id);
      this->trying_queue.erase(node);
    } else {
      node->value.matchingWith = node->value.matchingWith->next;
      node->value.progress = WAITING;
    }
    
    this->arrange_job();
  }
}



