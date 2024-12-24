#include "api.hpp"

using namespace postgres;
class LastModifiedHandler : public RequestHandler {
  private:
  static std::string build_query(const std::string& table_name) {
    return "SELECT last_modified FROM public.\"" + table_name + "\" ORDER BY last_modified DESC LIMIT 1";
  }

  /**
   * Select the last modified date of a table.
   * @param table_name Name of the table to select the last modified date from.
   * @param verbose Whether to print messages to stdout.
   * @return Last modified date of the table if found, "" otherwise.
   */
  std::string select_last_modified(const std::string& table_name, int verbose) {
    try {
      auto& pool = get_connection_pool();
      auto c = pool.acquire();
      pqxx::work txn(*c);
      std::string last_modified = txn.query_value<std::string>(build_query(table_name));
      txn.commit();
      pool.release(c);
      
      if (last_modified.empty()) {
        verbose && std::cerr << "Table " << table_name << " not found" << std::endl;
        return "";
      }
      return last_modified;
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    }
    return "";
  }

  public:
  std::string get_endpoint() const override {
    return "/api/last_modified";
  }

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req, const std::string& ip_address) {
    if (middleware::rate_limited(ip_address))
      return request::make_too_many_requests_response("Too many requests", req);

    std::string_view session_id = request::get_session_id_from_cookie(req);
    int user_id = request::select_user_data_from_session(session_id, 0).user_id;
    if (req.method() == http::verb::get) {
      /**
       * -------------- GET LAST MODIFIED --------------
       */

      std::optional<std::string> table_opt = request::parse_from_request(req, "table_name");

      if (!table_opt.has_value()) {
        return request::make_bad_request_response("Invalid parameters", req);
      }

      auto& table = table_opt.value();
      std::string * required_permissions = new std::string[1]{table + ".admin"};
      if (!middleware::check_permissions(request::get_user_permissions(user_id, 0), required_permissions, 1))
        return request::make_unauthorized_response("Unauthorized", req);

      std::string last_modified = select_last_modified(table, 0);
      if (last_modified.empty()) {
        return request::make_bad_request_response("Table not found", req);
      }

      nlohmann::json response_json;
      response_json["message"] = "Last modified date found successfully";
      response_json["last_modified"] = last_modified;
      return request::make_ok_request_response(response_json.dump(4), req);
    } else {
      return request::make_bad_request_response("Invalid method", req);
    }
  }
};

extern "C" RequestHandler* create_last_modified_handler() {
  return new LastModifiedHandler();
}