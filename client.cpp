#include "client.hpp"
#include <string>
#include <sstream>
#include <queue>
#include <string.h>

using namespace std;

int count_char(char *s, char c) {
    int len = strlen(s);
    int count = 0;
    for (int i = 0; i < len; i++) {
        if (s[i] == c) {
            count += 1;
        }
    }
    return count;
}

void Client::recv_message(char *s) {
    int cmd_number = count_char(s, '\n');
    this->buffer << s;
    for (int i = 0; i < cmd_number; i++) {
        string str;
        this->buffer >> str;
        this->cmd_queue.push(str);
    }
    return;
}