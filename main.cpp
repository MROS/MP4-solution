#include "json.hpp"
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

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

  fd_set active_fd_set;
  FD_ZERO(&active_fd_set);
  FD_SET(socket_fd, &active_fd_set);
  int max_fd = socket_fd;

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
    } else if (ret > 0) {
      for (int i = 0; i < FD_SETSIZE; i++) {
        if (FD_ISSET(i, &read_fds)) {
          if (i == socket_fd) {

            write(STDOUT_FILENO, "試圖連接\n", strlen("試圖連接\n"));
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
                     inet_ntoa(client_addr.sin_addr),
                     ntohs(client_addr.sin_port), new_fd);

              /* Add to fd set */
              FD_SET(new_fd, &active_fd_set);
              if (new_fd > max_fd)
                max_fd = new_fd;
            }
          } else {
            // 已連接之 socket

            char buf[8192];
            memset(buf, 0, sizeof(buf));

            int recv_len = recv(i, buf, sizeof(buf), 0);

            if (recv_len == -1) {
              perror("recv()");
              return -1;
            } else if (recv_len == 0) {
              printf("Client disconnect\n");
            } else {
              printf("Receive: len=[%d] msg=[%s]\n", recv_len, buf);
              send(i, buf, recv_len, 0);
            }
          }

        } // end of if
      }   // end of for
    }     // end of if
  }       // end of while
}
