#ifndef POSTGRES_HPP
#define POSTGRES_HPP

#include <pqxx/pqxx>
#include <iostream>
#include <sstream>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "config.h"

namespace postgres {
  class ConnectionPool {
  private:
    std::queue<pqxx::connection*> pool;
    std::mutex pool_mutex;
    std::condition_variable pool_cv;
    int max_size;
    
    pqxx::connection* create_new_connection();
    int validate_connection(pqxx::connection* c);
  public:
    explicit ConnectionPool(int size);
    ~ConnectionPool();

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    pqxx::connection* acquire();
    void release(pqxx::connection* c);
  };

  void init_connection();
  ConnectionPool& get_connection_pool();
}

#endif
