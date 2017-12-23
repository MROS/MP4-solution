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
	size_t len = mqAttr.mq_msgsize;
	char *s = new char[len];
	int err = mq_receive(mq_pair.from_main_mqd, s, len, NULL);
	if (err == -1) {
	  perror("mq_receive");
	} else {
	  printf("worker receive\n");
	  
	}
	
	// TODO: 工作
	
	err = mq_send(mq_pair.from_worker_mqd, "ok", 2, 0);
	if (err == -1) {
	  perror("mq_send");
	} else { printf("worker send\n"); }
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
  TryingUser trying_user;
  trying_user.id = id;
  trying_user.matchingWith = nullptr;
  this->trying_queue.push_back(trying_user);
  this->arrange_job();
}

void MatchQueue::arrange_job() {
  if (trying_queue.length == 0) { return; }

}
