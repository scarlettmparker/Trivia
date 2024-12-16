#include "api.hpp"

using namespace postgres;
class LogoutHandler : public RequestHandler {
  private:
  /**
   * Select the user ID from a session ID.
   * This is used to verify that the session ID matches the user ID in the request,
   * preventing users from logging out other users.
   *
   * @param session_id Session ID to select the user ID from.
   * @param verbose Whether to print messages to stdout.
   * @return User ID if the session is valid, -1 otherwise.
   */
  int select_user_id_from_session(std::string_view session_id, int verbose) {
    try {
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("select_user_id_from_session", session_id);
      txn.commit();
      if (r.empty()) {
        verbose && std::cerr << "Session ID " << session_id << " not found" << std::endl;
        return -1;
      }
      return r[0][0].as<int>();
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return -1;
  }
  
  public:
  std::string get_endpoint() const override {
    return "/api/logout";
  }

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req, const std::string& ip_address){
    if (req.method() == http::verb::post) {
      /**
       * -------------- LOGOUT USER --------------
       */

      auto json_request = nlohmann::json::object();
      try{
        json_request = nlohmann::json::parse(req.body());
      } catch (const nlohmann::json::parse_error& e) {
        return request::make_bad_request_response("Invalid JSON request", req);
      }

      if (!json_request.contains("user_id"))
        return request::make_bad_request_response("Invalid user id parameters", req);

      const int user_id = json_request["user_id"];
      std::string_view session_id = request::get_session_id_from_cookie(req);

      if (session_id.empty())
        return request::make_unauthorized_response("Invalid or expired session", req);
      if (select_user_id_from_session(session_id, 0) != user_id)
        return request::make_unauthorized_response("Session id does not match user id!", req);

      request::invalidate_session(session_id, 0);
      nlohmann::json response_json;
      response_json["message"] = "Logout successful";
      return request::make_ok_request_response(response_json.dump(4), req);
    } else {
      return request::make_bad_request_response("Invalid request method", req);
    }
  }
};

extern "C" RequestHandler* create_logout_handler() {
  return new LogoutHandler();
}