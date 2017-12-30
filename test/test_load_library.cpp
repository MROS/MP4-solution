#include "../user.hpp"
#include "../mq.hpp"
#include "../load_library_util.hpp"

#include <dlfcn.h>
#include <string.h>
#include <stdio.h>

int main() {
  
  Info info;
  
  info.id = 0;
  info.counter = 0;
  char filter_function_str[] = "int filter_function(struct User user) { return 1111; }";
  
  save_source(filter_function_str, info.id, info.counter);
  
  void *handle = get_library_handle(&info);
  
  printf("%d\n", run_filter(handle, info.user));
  
}