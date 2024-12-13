#include "postgres.hpp"

namespace postgres {
  pqxx::connection * c = nullptr;

  /**
   * Prepare the statements used by the program.
   * This speeds up the execution of queries by precompiling them,
   * and prevents SQL injection attacks by using prepared statements.
   */
  void prepare_statements() {
    pqxx::work txn(*c);

    /* Category Queries */
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
    txn.conn().prepare("invalidate_session",
      "UPDATE public.\"Sessions\" SET active = FALSE WHERE id = $1;");
    txn.conn().prepare("set_session_id",
      "INSERT INTO public.\"Sessions\" (id, user_id, created_at, last_accessed, expires_at, ip_address, active) "
      "VALUES ($1, $2, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP + ($3 || ' seconds')::interval, $4, TRUE) "
      "RETURNING id;");

    /* User Queries */
    txn.conn().prepare("select_user_id",
      "SELECT id from public.\"Users\" WHERE username = $1 LIMIT 1;");
    txn.conn().prepare("select_password", 
      "SELECT password FROM public.\"Users\" WHERE username = $1 LIMIT 1;");
    txn.commit();
  }

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
        prepare_statements();
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