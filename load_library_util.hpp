#ifndef LOAD_LIBRARY_UTIL_HPP
#define LOAD_LIBRARY_UTIL_HPP

#include "mq.hpp"
#include "user.hpp"

void *get_library_handle(Info *info);
int run_filter(void* handle, User user);
void save_source(char *filter_function, int id, unsigned int counter);
  
#endif