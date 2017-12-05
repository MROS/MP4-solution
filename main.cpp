#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "json.hpp"

int main() {
    int socket_fd = 0;
    socket_fd = socket(AF_INET , SOCK_STREAM , 0);

    if (socket_fd == -1) {
        printf("創建 socket 失敗");
    }

    struct sockaddr_in addr;

    bzero(&addr, sizeof(addr));
    addr.sin_family = PF_INET;
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_port = htons(9898);

    int err = bind(socket_fd,(struct sockaddr *)&addr,sizeof(addr));
    if(err==-1){
        printf("bind error");
    }

    listen(socket_fd, 1000);

    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int client_socket_fd = accept(socket_fd,(struct sockaddr*) &client_addr, &addrlen);
    while (true) {
        char buffer[1024];
        ssize_t len = recv(client_socket_fd, buffer, sizeof(buffer), 0);
        write(STDOUT_FILENO, buffer, len);
    }

    printf("斷線了\n");
}
