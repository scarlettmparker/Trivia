#ifndef POSTGRES_HPP
#define POSTGRES_HPP

#include <pqxx/pqxx>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <stdbool.h>

#include "config.h"

namespace postgres {
  extern pqxx::connection * c;

  void prepare_statements();
  void init_connection();
  void close_connection();

  int select_category(const char * category_name, int verbose = 0);
  int create_category(const char * category_name, int verbose = 0);
  int delete_category(const char * category_name, int verbose = 0);
}

#endif