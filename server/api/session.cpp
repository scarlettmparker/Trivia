#include "session.hpp"

using namespace postgres;
class SessionHandler : public RequestHandler {
  private:
  struct UserData {
    int user_id;
    std::string username;
  };

  std::mutex cache_mutex;
  std::unordered_map<std::string, UserData> session_cache;

  public:
  std::string get_endpoint() const override{
    return "/api/session";
  }

  /**
   * Invalidate a session by setting it to inactive.
   * @param session_id Session ID to invalidate.
   * @param verbose Whether to print messages to stdout.
   */
  void invalidate_session(const std::string& session_id, int verbose) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    session_cache.erase(session_id);
    try {
      pqxx::work txn(*postgres::c);
      txn.exec_prepared("invalidate_session", session_id);
      txn.commit();
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
  }

  /**
   * Get the session ID from a cookie in a request.
   * @param req Request to get the session ID from.
   * @return Session ID from the cookie.
   */
  std::string get_session_id_from_cookie(const http::request<http::string_body>& req) {
    auto cookie_header = req[http::field::cookie];
    std::string session_id;

    size_t pos = cookie_header.find("sessionId=");
    if (pos != std::string::npos) {
      session_id = std::string(cookie_header.substr(pos + 10));
    }
    return session_id;
  }

  /**
   * Select the user data from the session ID.
   * @param session_id Session ID to select the user ID from.
   * @param verbose Whether to print messages to stdout.
   * @return User data if the session is valid, {-1, ""} otherwise.
   */
  UserData select_user_data_from_session(const std::string& session_id, int verbose) {
    {
      std::lock_guard<std::mutex> lock(cache_mutex);
      auto it = session_cache.find(session_id);
      if (it != session_cache.end()) {
        return it->second;
      }
    }

    try {
      pqxx::work txn(*postgres::c);
      pqxx::result r = txn.exec_prepared("select_user_data_from_session", session_id);
      txn.commit();

      if (r.empty()) {
        verbose && std::cerr << "Session ID " << session_id << " not found" << std::endl;
        invalidate_session(session_id, 0);
        return {-1, ""};
      }

      int user_id = std::stoi(r[0][0].c_str());
      std::string username = r[0][1].c_str();
      UserData user_data = {user_id, username};
      {
        std::lock_guard<std::mutex> lock(cache_mutex);
        session_cache[session_id] = user_data;
      }
      return user_data;
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return {-1, ""};
  }

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req, const std::string& ip_address) {
    if (req.method() == http::verb::get) {
      /**
       * -------------- VALIDATE SESSION --------------
       */
      
      std::string session_id = get_session_id_from_cookie(req);
      if (session_id.empty()) {
        return request::make_unauthorized_response("Invalid or expired session", req);
      }

      UserData user_data = select_user_data_from_session(session_id, 0);
      if (user_data.user_id == -1) {
        return request::make_unauthorized_response("Invalid or expired session", req);
      }

      nlohmann::json response_json;
      response_json["message"] = "Session validated successfully";
      response_json["user_id"] = user_data.user_id;
      response_json["username"] = user_data.username;
      return request::make_ok_request_response(response_json.dump(4), req);
    } else {
      return request::make_bad_request_response("Invalid request method", req);
    }
  }
};

extern "C" RequestHandler* create_session_handler() {
  pqxx::work txn(*c);

  txn.conn().prepare("select_user_id_from_session",
    "SELECT user_id FROM public.\"Sessions\" WHERE id = $1 AND expires_at > NOW() AND active = TRUE LIMIT 1;");
  txn.conn().prepare("select_user_data_from_session",
    "SELECT user_id, username FROM public.\"Sessions\" WHERE id = $1 AND expires_at > NOW() AND active = TRUE LIMIT 1;");
  txn.conn().prepare("invalidate_session",
    "UPDATE public.\"Sessions\" SET active = FALSE WHERE id = $1;");

  txn.commit();
  return new SessionHandler();
}