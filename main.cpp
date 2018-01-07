#include "json.hpp"
#include "client.hpp"
#include "match_queue.hpp"
#include "mq.hpp"
#include "config.hpp"

#include <arpa/inet.h>
#include <map>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/signalfd.h>

using json = nlohmann::json;
using namespace std;

const size_t BUF_SIZE = 8192;

enum IpcType {
  SOCKET_ACCEPT,
  SOCKET_RECV,
  MSG_QUEUE,
  SIGNAL
};

int set_up_socket(uint16_t port) {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;
  
  if (socket_fd == -1) {
    fprintf(stderr, "創建 socket 失敗\n");
    return -1;
  }
  
  bzero(&addr, sizeof(struct sockaddr_in));
  addr.sin_family = PF_INET;
  addr.sin_addr.s_addr = inet_addr("0.0.0.0");
  addr.sin_port = htons(port);
  
  int err =
  bind(socket_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (err == -1) {
    perror("bind");
    return -1;
  }
  
  err = listen(socket_fd, 1000);
  if (err == -1) {
    fprintf(stderr, "listen error\n");
    return -1;
  }
  
  return socket_fd;
}

int set_up_signal_fd() {
  sigset_t blockSet;
  sigemptyset(&blockSet);
  sigaddset(&blockSet, SIGCHLD);
  
  if (sigprocmask(SIG_SETMASK, &blockSet, NULL) == -1) {
    perror("sigprocmask()");
    exit(1);
  }
  
  int fd = signalfd(-1, &blockSet, 0);
  if (fd == -1) {
    perror("signalfd()");
    exit(1);
  }
  return fd;
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    printf("用法： ./inf-bonbon-server [port]\n");
    return 1;
  }
  
  // NOTE: 除錯用，可能導致減速
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  int port = atoi(argv[1]);
  
  
  // 初始化 socket
  int socket_fd = set_up_socket(port);
  if (socket_fd == -1) {
    printf("socket fail\n");
    return 1;
  }
  printf("啓動 socket 成功，監聽 %d\n", port);
  
  pid_t pid = getpid();
  printf("main process pid: %d\n", pid);
  
  // 初始化 signal_fd
  int signal_fd = set_up_signal_fd();
  
  // 開始 select
  fd_set active_fd_set;
  FD_ZERO(&active_fd_set);
  FD_SET(socket_fd, &active_fd_set);
  FD_SET(signal_fd, &active_fd_set);
  int max_fd = socket_fd > signal_fd ? socket_fd : signal_fd;
  
  MQSet mq_set = create_MQ_pair();
  FD_SET(mq_set.from_worker_job_mqd, &active_fd_set);
  max_fd = (max_fd > mq_set.from_worker_job_mqd) ? max_fd : mq_set.from_worker_job_mqd;
  FD_SET(mq_set.from_worker_pid_mqd, &active_fd_set);
  max_fd = (max_fd > mq_set.from_worker_job_mqd) ? max_fd : mq_set.from_worker_pid_mqd;
  
  // 各個 fd 分別爲哪一種 IPC
  std::map<int, IpcType> ipc_type_map;
  // id 到 Client 類的映射
  std::map<int, Client*> clients;
  // fd 到 id 的映射
  std::map<int, int> fd_to_id;
  
  MatchQueue match_queue(mq_set, &clients);
  
  ipc_type_map[socket_fd] = SOCKET_ACCEPT;
  ipc_type_map[signal_fd] = SIGNAL;
  ipc_type_map[mq_set.from_worker_job_mqd] = MSG_QUEUE;
  ipc_type_map[mq_set.from_worker_pid_mqd] = MSG_QUEUE;
  // 唯一描述一個客戶端的識別子
  // NOTE: 在生產環境中，不應使用如此簡單的遞增策略來建立唯一識別子
  int unique_id_count = 0;
  
  while (true) {
    fd_set read_fds = active_fd_set;
    
    int ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
    
    if (ret == -1) {
      perror("select()");
      return -1;
    } else if (ret == 0) {
      printf("時間設爲 NULL ，本行不可能執行\n");
      continue;
    }
    
    // 正常
    
    for (int fd = 0; fd <= max_fd; fd++) {
      if (FD_ISSET(fd, &read_fds)) {
	switch (ipc_type_map[fd]) {
	  
	  case SOCKET_ACCEPT: {
	    /* Accept */
	    struct sockaddr_in client_addr;
	    socklen_t addrlen = sizeof(client_addr);
	    int new_fd =
	    accept(socket_fd, (struct sockaddr *)&client_addr, &addrlen);
	    
	    if (new_fd == -1) {
	      perror("accept()");
	      return -1;
	    } else {
	      printf("新客戶端來自 %s:%u ，檔案描述子 %d\n",
		     inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
		     new_fd);
	      
	      ipc_type_map[new_fd] = SOCKET_RECV;
	      fd_to_id[new_fd] = unique_id_count;
	      clients[unique_id_count] = new Client(new_fd, unique_id_count);
	      unique_id_count += 1;
	      
	      /* Add to fd set */
	      FD_SET(new_fd, &active_fd_set);
	      if (new_fd > max_fd) {
		max_fd = new_fd;
	      }
	    }
	    break;
	  }
	  case SOCKET_RECV: {
	    // 已連接之 socket
	    
	    char buf[BUF_SIZE];
	    memset(buf, 0, sizeof(buf));
	    
	    int recv_len = recv(fd, buf, sizeof(buf) - 1, 0);
	    
	    if (recv_len == -1 || recv_len == 0) {
	      if (recv_len == -1) {
	        perror("recv()");
	      }
	      int id = fd_to_id[fd];
	      Client *client = clients[id];
	      
	      // 與 quit 狀況類似，若修改此處，另一處也需要修改
	      if (client->status == TALKING) {
		int other_side_id = fd_to_id[client->match_fd];
		Client *other_side_client = clients[other_side_id];
		other_side_client->other_side_quit();
	      } else if (client->status == MATCHING) {
		match_queue.handle_quit(id);
	      }
	      
	      printf("Client disconnect\n");
	      if (close(fd) == -1) {
		perror("close()");
	      }
	      FD_CLR(fd, &active_fd_set);
	      
	      fd_to_id.erase(fd);
	      delete client;
	      clients.erase(id);
	      ipc_type_map.erase(fd);
	      
	    } else if (recv_len > 0) {
      
	      printf("接收訊號 長度=%d 訊息=%s\n", recv_len, buf);
	      int id = fd_to_id[fd];
	      Client *client = clients[id];
	      client->socket_recv(buf);
      
	      while (!client->cmd_queue.empty()) {

		string str = client->cmd_queue.front();
		client->cmd_queue.pop();
		json j;
		try {
		  j = json::parse(str);
		} catch (std::invalid_argument e) {}
		std::string comming_cmd;
		comming_cmd = j["cmd"].get<string>();

		if (comming_cmd == string("try_match")) {
  
		  client->try_match_ack(j);
		  match_queue.add(id);
  
		} else if (comming_cmd == string("send_message")) {
  
		  client->send_message(j);
  
		} else if (comming_cmd == string("quit")) {
  
		  // 與斷線狀況類似，若修改此處，另一處也需要修改
		  if (client->status == TALKING) {
		    int other_side_id = fd_to_id[client->match_fd];
		    Client *other_side_client = clients[other_side_id];
		    client->quit();
		    other_side_client->other_side_quit();
		  } else if (client->status == MATCHING) {
		    match_queue.handle_quit(id);
		    client->quit();
		  }

		} else {
  
		  cout << comming_cmd;
		  printf("不合規格的指令\n");
  
		}
	      }
	    }
	    break;
	  }
	  case MSG_QUEUE: {
	    if (fd == mq_set.from_worker_job_mqd) {
	      ReportJob report_job;
	      mq_receive(mq_set.from_worker_job_mqd, (char *)&report_job, sizeof(ReportJob), NULL);
	      match_queue.handle_report(report_job);
	    } else if (fd == mq_set.from_worker_pid_mqd) {
	      ReportPid report_pid;
	      mq_receive(mq_set.from_worker_pid_mqd, (char *)&report_pid, sizeof(ReportPid), NULL);
	      match_queue.handle_report_pid(report_pid);
	    }
	    break;
	  }
	  case SIGNAL: {
	    // 無法確定是在收到 report_pid 之後才收到 SIGCHLD
	    struct signalfd_siginfo s;
	    int len = read(fd, &s, sizeof(struct signalfd_siginfo));
	    if (len != sizeof(struct signalfd_siginfo)) {
	      perror("read signal fd");
	    }
	    
	    int pid;
	    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
	      printf("child pid %d die\n", pid);
	      match_queue.handle_child_crash(pid);
	    }
	  }
	}
      }
    }
  }
}
