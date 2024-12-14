#include "postgres.hpp"

namespace postgres {
  pqxx::connection * c = nullptr;

  /*
   * Initialize the connection to the PostgreSQL database.
   */
  void init_connection() {
    if (!c) {
      char connection_str[512];
      snprintf(connection_str, sizeof(connection_str),
        "user=%s password=%s host=%s port=%s dbname=%s target_session_attrs=read-write",
        TRIVIA_DB_USERNAME, TRIVIA_DB_PASSWORD, TRIVIA_DB_HOST, TRIVIA_DB_PORT, TRIVIA_DB_NAME);
      c = new pqxx::connection(connection_str);
      
      if (!c->is_open()) {
        delete c;
        c = nullptr;
        throw std::runtime_error("Failed to open PostgreSQL connection!.");
      } else {
        std::cout << "Connected to PostgreSQL database " << TRIVIA_DB_NAME
          << std::endl << "At address: " << TRIVIA_DB_HOST << ", port: " << TRIVIA_DB_PORT << std::endl;
      }
    }
  }

  /*
   * Close the connection to the PostgreSQL database.
   */
  void close_connection() {
    if (c) {
      delete c;
      c = nullptr;
    }
  }
}