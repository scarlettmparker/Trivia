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
    txn.conn().prepare("select_category", 
      "SELECT id FROM public.\"Category\" WHERE category_name = $1 LIMIT 1;");
    txn.conn().prepare("create_category", 
      "INSERT INTO public.\"Category\" (category_name) VALUES ($1) "
      "ON CONFLICT (category_name) DO NOTHING RETURNING id;");
    txn.conn().prepare("delete_category", 
      "DELETE FROM public.\"Category\" WHERE category_name = $1 RETURNING id;");
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

  /**
   * Select a category by name from the database.
   * @param category_name Name of the category to select.
   * @param verbose Whether to print messages to stdout.
   * @return ID of the category if found, 0 otherwise.
   */
  int select_category(const std::string& category_name, bool verbose) {
    try {
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("select_category", category_name);
      if (r.empty()) {
        verbose && std::cout << "Category " << category_name << " not found" << std::endl;
        return 0;
      }
      verbose && std::cout << "Category " << category_name << " found" << std::endl;
      return r[0][0].as<int>();
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    }
    return 0;
  }
  
  /**
   * Create a category in the database.
   * @param category_name Name of the category to create.
   * @param verbose Whether to print messages to stdout.
   * @return 1 if the category was created, 0 otherwise.
   */
  int create_category(const char * category_name, int verbose) {
    try {
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("create_category", category_name);
      txn.commit();

      if (!r.empty()) {
        verbose && std::cout << "Successfully created category " << category_name << std::endl;
        return 1;
      } else {
        verbose && std::cerr << "Category " << category_name << " already exists" << std::endl;
        return 0;
      }
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return 0;
  }

  /**
   * Delete a category from the database.
   * @param category_name Name of the category to delete.
   * @param verbose Whether to print messages to stdout.
   * @return 1 if the category was deleted, 0 otherwise.
   */
  int delete_category(const char * category_name, int verbose) {
    try {
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("delete_category", category_name);
      txn.commit();

      if (!r.empty()) {
        verbose && std::cout << "Successfully deleted category with name " << category_name << std::endl;
        return 1;
      } else {
        verbose && std::cerr << "Category with name " << category_name << " does not exist" << std::endl;
        return 0;
      }
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return 0;
  }
}