#include "session.hpp"

using namespace postgres;
class SessionHandler : public RequestHandler {
  private:
  struct UserData {
    int user_id;
    std::string username;
  };
  
  void cleanup_cache() {
    std::lock_guard<std::mutex> lock(request::cache_mutex);
    auto now = std::chrono::system_clock::now();
    
    for (auto it = request::session_cache.begin(); it != request::session_cache.end();) {
      if (it->second.expiry < now) {
        it = request::session_cache.erase(it);
      } else {
        ++it;
      }
    }

    // if cache is too large, remove the oldest entries
    if (request::session_cache.size() > request::MAX_CACHE_SIZE) {
      auto oldest = request::session_cache.begin();
      for (auto it = request::session_cache.begin(); it != request::session_cache.end(); ++it) {
        if (it->second.expiry < oldest->second.expiry) {
          oldest = it;
        }
      }
      request::session_cache.erase(oldest);
    }
  }

  /**
   * Select the user data from the session ID.
   * @param session_id Session ID to select the user ID from.
   * @param verbose Whether to print messages to stdout.
   * @return User data if the session is valid, {-1, ""} otherwise.
   */
  UserData select_user_data_from_session(const std::string& session_id, int verbose) {
    {
      std::lock_guard<std::mutex> lock(request::cache_mutex);
      auto it = request::session_cache.find(session_id);
      if (it != request::session_cache.end()) {
        auto now = std::chrono::system_clock::now();
        if (it->second.expiry > now) {
          return {it->second.user_id, it->second.username};
        }
        request::session_cache.erase(it);
      }
    }

    try {
      pqxx::work txn(*postgres::c);
      pqxx::result r = txn.exec_prepared("select_user_data_from_session", session_id);
      txn.commit();

      if (r.empty()) {
        verbose && std::cerr << "Session ID " << session_id << " not found" << std::endl;
        request::invalidate_session(session_id, 0);
        return {-1, ""};
      }

      int user_id = std::stoi(r[0][0].c_str());
      std::string username = r[0][1].c_str();
      
      {
        std::lock_guard<std::mutex> lock(request::cache_mutex);
        auto expiry = std::chrono::system_clock::now() + std::chrono::seconds(request::CACHE_TTL_SECONDS);
        request::session_cache[session_id] = {user_id, username, expiry};
        
        if (request::session_cache.size() > request::MAX_CACHE_SIZE) {
          cleanup_cache();
        }
      }
      
      return {user_id, username};
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return {-1, ""};
  }

  public:
  std::string get_endpoint() const override{
    return "/api/session";
  }

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req, const std::string& ip_address) {
    if (req.method() == http::verb::get) {
      /**
       * -------------- VALIDATE SESSION --------------
       */
      std::string session_id = request::get_session_id_from_cookie(req);
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