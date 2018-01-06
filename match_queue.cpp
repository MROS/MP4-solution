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
  report.candidate_id = assign_job->candidate_info.id;
  
  void *handle = get_library_handle(&(assign_job->trying_info));
  int result = run_filter(handle, assign_job->candidate_info.user);
  dlclose(handle);
  if (result == 0) {
    report.result = false;
    return report;
  }

  handle = get_library_handle(&(assign_job->candidate_info));
  result = run_filter(handle, assign_job->trying_info.user);
  dlclose(handle);
  if (result == 0) {
    report.result = false;
    return report;
  }

  report.result = true;
  return report;
}

void worker(MQSet mq_set) {
  pid_t my_pid = getpid();
  pid_t ppid = getppid();
  printf("%d fork by %d\n", my_pid, ppid);
  while (true) {
    AssignJob assign_job;
    int err = mq_receive(mq_set.from_main_mqd, (char*)&assign_job, sizeof(AssignJob), NULL);
    if (err == -1) {
      perror("mq_receive");
    }
    ReportPid report_pid;
    report_pid.pid = my_pid;
    report_pid.trying_id = assign_job.trying_info.id;
    report_pid.candidate_id = assign_job.candidate_info.id;
    mq_send(mq_set.from_worker_pid_mqd, (char*)&report_pid, sizeof(ReportPid), 0);
    
    printf("試圖匹配 %d %d\n", assign_job.trying_info.id, assign_job.candidate_info.id);
    ReportJob report = do_job(&assign_job);
    report.pid = my_pid;
    
    err = mq_send(mq_set.from_worker_job_mqd, (char*)&report, sizeof(ReportJob), 0);
    if (err == -1) {
      perror("mq_send");
    }
  }
}

MatchQueue::MatchQueue(MQSet mq_set, map<int, Client*>* clients) {
  this->clients = clients;
  this->mq_set = mq_set;
  this->working_job = 0;
  
  for (int i = 0; i < WORKER_NUMBER; i++) {
    this->new_child();
  }
}

void MatchQueue::new_child() {
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork()");
  }
  else if (pid == 0) {
    // 子進程
    worker(this->mq_set);
  }
}


void MatchQueue::handle_child_crash(pid_t child_pid) {
  this->new_child();
  
  auto it = this->job_record.find(child_pid);
  if (it == this->job_record.end()) {
    // 還未收到 ReportPid
    this->killed_child.insert(child_pid);
  } else {
    ReportJob report_job;
    report_job.result = false;
    report_job.pid = child_pid;
    report_job.trying_id = this->job_record[child_pid].trying_id;
    report_job.candidate_id = this->job_record[child_pid].candidate_id;
    this->handle_report(report_job);
  }
}

void MatchQueue::detach(int id) {
  
  this->quit_set.insert(id);
  
  Node<TryingUser> *trying_node = this->trying_queue.search([id](TryingUser user) { return user.id == id; });
  
  if (trying_node != nullptr) {
    printf("detach: 欲 quit 之客戶端 %d 位於 trying\n", id);
    this->trying_queue.erase(trying_node);
    this->arrange_job();
    return;
  }
  
  Node<int> *candidate_node = this->candidate_queue.search([id](int iter_id) { return id == iter_id; });
  
  if (candidate_node != nullptr) {
    printf("detach: 欲 quit 之客戶端 %d 位於 candidate\n", id);
    
    bool is_tail = (candidate_node == this->candidate_queue.tail);
    bool trying_empty = (this->trying_queue.length == 0);
    
    Node<TryingUser> *iter = this->trying_queue.search([candidate_node](TryingUser user) { return user.matchingWith == candidate_node; });
    
    if (is_tail) {
      printf("in tail\n");
      if (iter != nullptr && iter->value.matchingWith == candidate_node) {
	this->candidate_queue.push_back(iter->value.id);
	auto next_iter = iter->next;
	this->trying_queue.erase(iter);
	iter = next_iter;
      }
    }
    
    Node<int> *next_candidate = candidate_node->next;
    while (iter != nullptr && iter->value.matchingWith == candidate_node) {
      iter->value.matchingWith = next_candidate;
      iter = iter->next;
    }
    this->candidate_queue.erase(candidate_node);
    printf("detach %d 結束\n", id);
    
    this->arrange_job();
  } else {
    printf("欲 detach 的人不在隊伍中\n");
  }
}


void MatchQueue::detach_with_matcher(Node<TryingUser>* trying_node) {
  
  Node<int>* candidate_node = trying_node->value.matchingWith;
  
  if (trying_node == nullptr || candidate_node == nullptr) {
    printf("試圖在隊伍中丟掉爲 nullptr 的節點\n");
    exit(1);
  }
  
  bool is_tail = (candidate_node == this->candidate_queue.tail);
  
  Node<TryingUser> *iter = trying_node->next;
  if (is_tail) {
    if (iter != nullptr && iter->value.matchingWith == candidate_node) {
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
  printf("id: %d, 伺服器處理匹配中 quit\n", id);
  this->detach(id);
}

void MatchQueue::handle_match(int id) {
 
  Node<TryingUser> *trying_node = this->trying_queue.search([id](TryingUser user) { return user.id == id; });
 
  if (trying_node != nullptr) {
 
    TryingUser trying_user = trying_node->value;
    Client *client = (this->clients->operator[](trying_user.id));

    // handle_match 會傳送 matched 到雙方
    int candidate_id = trying_user.matchingWith->value;
    client->handle_match(this->clients->operator[](candidate_id));
    
    this->detach_with_matcher(trying_node);
    
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
  
  while (iter != nullptr && this->working_job < WORKER_NUMBER) {
  // while (iter != nullptr) {
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
      
      mq_send(this->mq_set.from_main_mqd, (char *)&assign_job, sizeof(assign_job), 0);
      this->working_job += 1;
    }
    iter = iter->next;
  }
}

void MatchQueue::handle_report(ReportJob report) {
  this->working_job -= 1;
  this->job_record.erase(report.pid);
  
  if (report.result == true) {
    // TODO: 處理已經 quit 的狀況
    // 在測試中並不會發生
    this->handle_match(report.trying_id);
    this->arrange_job();
  } else if (report.result == false) {
    
    
    int trying_id = report.trying_id;
    int candidate_id = report.candidate_id;
    printf("%d %d 匹配結果爲 false\n", trying_id, candidate_id);
    
    // 若在 quit_set 中，不需要進行任何操作
    auto it = this->quit_set.find(trying_id);
    if (it != this->quit_set.end()) {
      this->quit_set.erase(it);
      this->arrange_job();
      return;
    }
    it = this->quit_set.find(candidate_id);
    if (it != this->quit_set.end()) {
      this->quit_set.erase(it);
      this->arrange_job();
      return;
    }
    
    Node<TryingUser> *node = this->trying_queue.search([trying_id](TryingUser user) { return user.id == trying_id; });
    
    if (node->value.matchingWith == this->candidate_queue.tail) {
      this->candidate_queue.push_back(trying_id);
      this->trying_queue.erase(node);
    } else {
      node->value.matchingWith = node->value.matchingWith->next;
      node->value.progress = WAITING;
    }
    
    this->arrange_job();
  }
}

void MatchQueue::handle_report_pid(ReportPid report_pid) {
  // XXX: 假使子進程 pid 不可能這麼快重複
  printf("pid %d handle %d %d match\n", report_pid.pid, report_pid.trying_id, report_pid.candidate_id);
  auto it = this->killed_child.find(report_pid.pid);
  
  if (it != this->killed_child.end()) {
    // 該子進程已死
    this->killed_child.erase(it);
    ReportJob report_job;
    report_job.pid = report_pid.pid;
    report_job.result = false;
    report_job.trying_id = report_pid.trying_id;
    report_job.candidate_id = report_pid.candidate_id;
    this->handle_report(report_job);
  } else {
    // 該子進程還活著
    this->job_record[report_pid.pid] = report_pid;
  }
}

