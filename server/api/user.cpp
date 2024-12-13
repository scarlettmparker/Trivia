#include "category.hpp"

using namespace postgres;
class UserHandler : public RequestHandler {
  public:
  std::string get_endpoint() const override{
    return "/api/user";
  }

  /**
   * Generate a session ID for a user. This function uses OpenSSL to generate a random 128-bit session ID.
   * @param verbose Whether to print messages to stdout.
   * @return Session ID as a string.
   */
  std::string generate_session_id(int verbose) {
    unsigned char buffer[16];
    if (RAND_bytes(buffer, sizeof(buffer)) != 1) {
      verbose && std::cerr << "Failed to generate session ID" << std::endl;
    }
    std::stringstream session_id;
    for (int i = 0; i < 16; ++i) {
      session_id << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i];
    }
    return session_id.str();
  }

  /**
   * Set a session cookie for a user. This ets the session ID in a cookie with the following attributes:
   * - HttpOnly: The cookie is not accessible via JavaScript.
   * - Secure: The cookie is only sent over HTTPS.
   * - SameSite=Strict: The cookie is not sent on cross-site requests.
   * - Max-Age=86400: The cookie expires after 24 hours.
   *
   * @param session_id Session ID to set in the cookie.
   * @return HTTP response with the session cookie set.
   */
  http::response<http::string_body> set_session_cookie(const std::string& session_id) {
    http::response<http::string_body> res{http::status::ok, 11};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::set_cookie, "sessionId=" + session_id + "; HttpOnly; Secure; SameSite=Strict Max-Age=86400");
    res.body() = R"({"message": "Login successful"})";
    res.prepare_payload();
    return res;
  }

  /**
   * Set a session ID for a user.
   * @param session_id Session ID to set.
   * @param user_id ID of the user to set the session ID for.
   * @param duration Duration of the session in seconds.
   * @param ip_address IP address of the user.
   * @param verbose Whether to print messages to stdout.
   * @return 1 if the session ID was set, 0 otherwise.
   */
  int set_session_id(std::string session_id, int user_id, int duration, std::string ip_address, int verbose) {
    try{
      pqxx::work txn(*c);
      txn.exec_prepared("set_session_id", session_id, user_id, duration, ip_address);
      txn.commit();
      return 1;
    } catch (const std:: exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return 0;
  }

  /**
   * Select the ID of a user by username.
   * @param username Username of the user to select.
   * @param verbose Whether to print messages to stdout.
   * @return ID of the user if found, -1 otherwise.
   */
  int select_user_id(const char * username, int verbose) {
    try{
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("select_user_id", username);
      if (r.empty()) {
        verbose && std::cout << "User with username " << username << " not found" << std::endl;
        return -1;
      }
      return r[0][0].as<int>();
    } catch (const std:: exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return -1;
  }

  /**
   * Select the password of a user by username.
   * @param username Username of the user to select.
   * @param verbose Whether to print messages to stdout.
   * @return (Hashed) password of the user if found, NULL otherwise.
   */
  const char * select_password(const char * username, int verbose) {
    try{
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("select_password", username);
      if (r.empty()) {
        verbose && std::cout << "User with username " << username << " not found" << std::endl;
        return NULL;
      }
      return r[0][0].c_str();
    } catch (const std:: exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return NULL;
  }

  /**
   * Authenticate a user with a username and password.
   * This function uses BCrypt to validate the password against the hashed password stored in the database.
   *
   * @param username Username of the user to authenticate.
   * @param password Password of the user to authenticate.
   * @param verbose Whether to print messages to stdout.
   * @return 1 if the user is authenticated, 0 otherwise.
   */
  int login(const char * username, const char * password, int verbose) {
    if (select_password(username, 0) == NULL) {
      std::cout << "User with username " << username << " not found" << std::endl;
      return 0;
    }

    if (BCrypt::validatePassword(password, select_password(username, 0))) {
      return 1;
    } else {
      verbose && std::cout << "Invalid password" << std::endl;
      return 0;
    }
  }

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req, const std::string& ip_address){
    if (req.method() == http::verb::post) {
      /**
      * -------------- LOGIN USER --------------
      */

      auto json_request = nlohmann::json::object();
      try {
        json_request = nlohmann::json::parse(req.body());
      } catch (const nlohmann::json::parse_error& e) {
        return request::make_bad_request_response("Invalid JSON request", req);
      }

      // validation
      if (!json_request.contains("username") || !json_request.contains("password"))
        return request::make_bad_request_response("Invalid request: Missing required fields (username | password).", req);
      if (!json_request["username"].is_string() || !json_request["password"].is_string())
        return request::make_bad_request_response("Invalid request: 'username' and 'password' must be strings.", req);

      const char *username = json_request["username"].get<std::string>().c_str();
      const char *password = json_request["password"].get<std::string>().c_str();

      if (!login(username, password, 0))
        return request::make_bad_request_response("Invalid username or password", req);

      // get user_id, generate session_id, set session_id, set session cookie
      std::string session_id = generate_session_id(0);
      int user_id = select_user_id(username, 0);
      int expires_at = 86400; // in seconds

      if (!set_session_id(session_id, user_id, expires_at, ip_address, 0))
        return request::make_bad_request_response("An unexpected error has occured.", req);

      return set_session_cookie(session_id);
    } else {
      return request::make_bad_request_response("Invalid request method", req);
    }
  }
};

extern "C" RequestHandler* create_user_handler() {
  return new UserHandler();
}