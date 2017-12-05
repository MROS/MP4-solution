#include "json.hpp"
#include "client.hpp"

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

using json = nlohmann::json;

const size_t BUF_SIZE = 8192;

enum IpcType {
  SOCKET_ACCEPT,
  SOCKET_RECV,
  MSG_QUEUE,
};

int set_up_socket(uint16_t port) {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;
  
  if (socket_fd == -1) {
    printf("創建 socket 失敗\n");
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
    printf("listen error\n");
    return -1;
  }
  
  return socket_fd;
}

int main(int argc, char *argv[]) {
  
  if (argc != 2) {
    printf("用法： ./inf-bonbon-server [port]\n");
    return 1;
  }
  
  int port = atoi(argv[1]);
  
  // 初始化 socket
  int socket_fd = set_up_socket(port);
  if (socket_fd == -1) {
    printf("啓動 socket 失敗，伺服器關閉\n");
    return 1;
  }
  printf("啓動 socket 成功，監聽 %d\n", port);
  
  // 開始 select
  fd_set active_fd_set;
  FD_ZERO(&active_fd_set);
  FD_SET(socket_fd, &active_fd_set);
  int max_fd = socket_fd;
  
  std::map<int, IpcType> ipc_type_map;
  std::map<int, Client*> clients;
  ipc_type_map[socket_fd] = SOCKET_ACCEPT;
  
  while (true) {
    /* Copy fd set */
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
    
    for (int i = 0; i <= max_fd; i++) {
      if (FD_ISSET(i, &read_fds)) {
	switch (ipc_type_map[i]) {
	  
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
	      
	      /* Add to fd set */
	      ipc_type_map[new_fd] = SOCKET_RECV;
	      clients[new_fd] = new Client(new_fd);
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
	    
	    int recv_len = recv(i, buf, sizeof(buf) - 1, 0);
	    
	    if (recv_len == -1) {
	      perror("recv()");
	      return -1;
	    } else if (recv_len == 0) {
	      printf("Client disconnect\n");
	      close(i);
	      FD_CLR(i, &active_fd_set);
	      delete clients[i];
	      clients.erase(i);
	      ipc_type_map.erase(i);
	    } else {
	      printf("Receive: len=[%d] msg=[%s]\n", recv_len, buf);
	      Client *client = clients[i];
	      client->recv_message(buf);
	      while (!client->cmd_queue.empty()) {
		string str = client->cmd_queue.front();
		client->cmd_queue.pop();
		json j;
		try {
		  j = json::parse(str);
		} catch (std::invalid_argument e) {}
		std::string cmd = j["cmd"].get<std::string>();
		if (cmd == string("try_match")) {
		  printf("try_match\n");
		} else if (cmd == string("send_message")) {
		  printf("send_message\n");
		} else if (cmd == string("quit")) {
		  printf("quit\n");
		} else {
		  cout << cmd;
		  printf("莫名其妙\n");
		}
	      }
	      break;
	    }
	  }
	  case MSG_QUEUE: {
	    // TODO: MESSAGE_QUEUE
	    break;
	  }
	  }
	}
      }
    }
  }