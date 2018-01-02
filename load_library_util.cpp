#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string>
#include <string.h>

#include "mq.hpp"
#include "user.hpp"

using namespace std;

void save_source_and_compile(char *filter_function, int id, unsigned int counter) {
  string trying_prefix = "./libfilter_" + to_string(id) + "_" + to_string(counter);
  string trying_source = trying_prefix + ".c";
  string trying_shared_library = trying_prefix + ".so";
  
  char user_structure_str[] = 
  "struct User {\n"
  "\tchar name[33];\n"
  "\tunsigned int age;\n"
  "\tchar gender[7];\n"
  "\tchar introduction[1025];\n"
  "};\n";
  
  int fd = open(trying_source.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (fd < 0) {
    perror("open trying_source");
  }
  write(fd, user_structure_str, strlen(user_structure_str));
  write(fd, filter_function, strlen(filter_function));
  close(fd);
  
  // TODO: 若在子進程中編譯，需要處理同步問題（一個正在編譯，一個已經開始讀取）
  // 編譯
  char command[1024];
  sprintf(command,
	  "gcc -fPIC -O2 -std=c11 %s -shared -o %s",
	  trying_source.c_str(),
	  trying_shared_library.c_str());
  system(command);
}

// 回傳 dlopen 的 handle
void *get_library_handle(Info *info) {
  string trying_prefix = "./libfilter_" + to_string(info->id) + "_" + to_string(info->counter);
  string trying_shared_library = trying_prefix + ".so";

  void *handle = dlopen(trying_shared_library.c_str(), RTLD_LAZY);
  if (handle == NULL) {
    printf("打開共享函式庫 %s 失敗: %s\n", trying_shared_library.c_str(), dlerror());
  }
  return handle;
}

int run_filter(void* handle, User user) {
  
  int (*filter_function)(User user) = (int (*)(User user)) dlsym(handle, "filter_function");
  
  const char *dlsym_error = dlerror();
  if (dlsym_error) {
    fprintf(stderr, "Cannot load symbol 'filter_function': %s\n", dlsym_error);
    dlclose(handle);
    return false;
  }
  
  return filter_function(user);
}