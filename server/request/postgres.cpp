#include "postgres.hpp"

namespace postgres {
  static ConnectionPool* global_pool = nullptr;
  std::unordered_map<pqxx::connection*, std::chrono::time_point<std::chrono::steady_clock>> last_used;

  /**
   * Create a new connection for the connection pool.
   * @return New connection.
   */
  pqxx::connection* ConnectionPool::create_new_connection() {
    auto c = new pqxx::connection(
      "user=" + std::string(TRIVIA_DB_USERNAME) +
      " password=" + std::string(TRIVIA_DB_PASSWORD) +
      " host=" + std::string(TRIVIA_DB_HOST) +
      " port=" + std::string(TRIVIA_DB_PORT) +
      " dbname=" + std::string(TRIVIA_DB_NAME) +
      " target_session_attrs=read-write" +
      " keepalives=1" +
      " keepalives_idle=30"
    );

    if (!c->is_open()) {
      delete c;
      throw std::runtime_error("Failed to open PostgreSQL connection!");
    }

    pqxx::work txn(*c);

    /* Category Queries */
    txn.conn().prepare("select_category_names_pagington",
      "SELECT category_name, id FROM public.\"Category\" ORDER BY category_name ASC LIMIT $1 OFFSET $2;");
    txn.conn().prepare("create_category", 
      "INSERT INTO public.\"Category\" (category_name) VALUES ($1) "
      "ON CONFLICT (category_name) DO NOTHING RETURNING id;");
    txn.conn().prepare("delete_category", 
      "DELETE FROM public.\"Category\" WHERE category_name = $1 RETURNING id;");

    /* Question Queries */
    txn.conn().prepare("select_question", 
      "SELECT id FROM public.\"Question\" WHERE id = $1 LIMIT 1;");
    txn.conn().prepare("create_question", 
      "INSERT INTO public.\"Question\" (question, answers, correct_answer, category_id) "
      "VALUES ($1, $2, $3, $4);");
    txn.conn().prepare("delete_question", 
      "DELETE FROM public.\"Question\" WHERE id = $1 RETURNING id;");

    /* Session Queries */
    txn.conn().prepare("select_user_id_from_session",
      "SELECT user_id FROM public.\"Sessions\" WHERE id = $1 AND expires_at > NOW() AND active = TRUE LIMIT 1;");
    txn.conn().prepare("select_user_data_from_session",
      "SELECT user_id, username FROM public.\"Sessions\" WHERE id = $1 AND expires_at > NOW() AND active = TRUE LIMIT 1;");
    txn.conn().prepare("invalidate_session",
      "UPDATE public.\"Sessions\" SET active = FALSE WHERE id = $1;");
    txn.conn().prepare("set_session_id",
      "INSERT INTO public.\"Sessions\" (id, user_id, username, created_at, last_accessed, expires_at, ip_address, active) "
      "VALUES ($1, $2, $3, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP + ($4 || ' seconds')::interval, $5, TRUE) "
      "RETURNING id;");

    /* User Queries */
    txn.conn().prepare("select_user_id",
      "SELECT id from public.\"Users\" WHERE username = $1 LIMIT 1;");
    txn.conn().prepare("select_username_from_id",
      "SELECT username from public.\"Users\" WHERE id = $1 LIMIT 1;");
    txn.conn().prepare("select_password", 
      "SELECT password FROM public.\"Users\" WHERE username = $1 LIMIT 1;");
    txn.conn().prepare("get_user_permissions",
      "SELECT p.id, p.permission_name FROM public.\"UserPermissions\" up "
      "JOIN public.\"Permissions\" p ON up.permission_id = p.id WHERE up.user_id = $1;");

    txn.commit();
    return c;
  }

  /**
   * Create a new connection pool with a given size.
   * @param size Size of the connection pool.
   */
  ConnectionPool::ConnectionPool(int size) : max_size(size) {
    for (int i = 0; i < size; ++i) {
      pool.push(create_new_connection());
    }
  }

  /**
   * Destroy the connection pool and free all connections.
   */
  ConnectionPool::~ConnectionPool() {
    while (!pool.empty()) {
      delete pool.front();
      pool.pop();
    }
  }

  /**
   * Validate a connection by executing a simple query.
   * @param conn Connection to validate.
   */
  int ConnectionPool::validate_connection(pqxx::connection* c) {
    try {
      pqxx::work txn(*c);
      txn.exec("SELECT 1");
      txn.commit();
      return true;
    } catch (...) {
      return false;
    }
  }

  /**
   * Acquire a connection from the pool.
   * @return Connection from the pool.
   */
  pqxx::connection* ConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(pool_mutex);
    pool_cv.wait(lock, [this] { return !pool.empty(); });

    auto c = pool.front();
    pool.pop();

    auto now = std::chrono::steady_clock::now();

    // check if the connection is still valid every minute
    if (last_used.find(c) == last_used.end() || 
      std::chrono::duration_cast<std::chrono::minutes>(now - last_used[c]).count() > 1) {
      if (!validate_connection(c)) {
        delete c;
        c = create_new_connection();
      }
    }

    last_used[c] = now;
    return c;
  }

  /**
   * Release a connection back to the pool.
   * @param c Connection to release.
   */
  void ConnectionPool::release(pqxx::connection* c) {
    std::lock_guard<std::mutex> lock(pool_mutex);
    pool.push(c);
    pool_cv.notify_one();
  }

  /**
   * Initialize the global connection pool.
   */
  void init_connection() {
    if (!global_pool) {
      global_pool = new ConnectionPool(5);
    }
  }

  /** 
   * Get the global connection pool.
   * @return Global connection pool.
   */
  ConnectionPool& get_connection_pool() {
    if (!global_pool) {
      throw std::runtime_error("Connection pool not initialized. Call init_connection first.");
    }
    return *global_pool;
  }
}
