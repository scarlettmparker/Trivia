#include "category.hpp"

using namespace postgres;
class UserHandler : public RequestHandler {
  private:
  
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
   * - SameSite=None: The cookie is sent with cross-site requests.
   * - Max-Age=86400: The cookie expires after 24 hours.
   *
   * @param session_id Session ID to set in the cookie.
   * @return HTTP response with the session cookie set.
   */
  http::response<http::string_body> set_session_cookie(const std::string& session_id) {
    http::response<http::string_body> res{http::status::ok, 11};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::set_cookie, "sessionId=" + session_id + "; HttpOnly; Secure; SameSite=None; Max-Age=86400");
    res.body() = R"({"message": "Login successful", "status": "ok"})";
    res.prepare_payload();
    return res;
  }

  /**
   * Set a session ID for a user.
   * @param session_id Session ID to set.
   * @param user_id ID of the user to set the session ID for.
   * @param username Username of the user to set the session ID for.
   * @param duration Duration of the session in seconds.
   * @param ip_address IP address of the user.
   * @param verbose Whether to print messages to stdout.
   * @return 1 if the session ID was set, 0 otherwise.
   */
  int set_session_id(std::string session_id, int user_id, std::string username, int duration, std::string ip_address, int verbose) {
    try{
      pqxx::work txn(*c);
      txn.exec_prepared("set_session_id", session_id, user_id, username, duration, ip_address);
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
   * Select a user by ID.
   * @param id ID of the user to select.
   * @param verbose Whether to print messages to stdout.
   * @return Username of the user if found, NULL otherwise.
   */
  std::string select_username_from_id(int id, int verbose) {
    try{
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("select_username_from_id", id);
      if (r.empty()) {
        verbose && std::cout << "User with ID " << id << " not found" << std::endl;
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
  int login(const char * username, const char * password) {
    const char* stored_password = select_password(username, 0);
    if (stored_password == NULL) {
      return 0;
    }
    return BCrypt::validatePassword(password, stored_password) ? 1 : 0;
  }

  public:
  std::string get_endpoint() const override{
    return "/api/user";
  }

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req, const std::string& ip_address){
    if (req.method() == http::verb::get) {
      /**
       * -------------- GET USER --------------
       */
      auto user_id_request = request::parse_from_request(req, "user_id");
      if (!user_id_request) {
        return request::make_bad_request_response("Invalid user id parameters", req);
      }

      std::string user = *user_id_request;
      int user_id;
      try {
        user_id = std::stoi(user);
      } catch (const std::invalid_argument& e) {
        return request::make_bad_request_response("Invalid user id format", req);
      } catch (const std::out_of_range& e) {
        return request::make_bad_request_response("User id out of range", req);
      }

      nlohmann::json response_json;
      std::string username = select_username_from_id(user_id, 0);
      if (username.empty())
        return request::make_bad_request_response("User not found", req);
      
      response_json["message"] = "User found successfully";
      response_json["user_id"] = user_id;
      response_json["username"] = username;
      return request::make_ok_request_response(response_json.dump(4), req);
    } else if (req.method() == http::verb::post) {
      /**
      * -------------- LOGIN USER --------------
      */

      nlohmann::json json_request;
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

      std::string_view username_str = json_request["username"].get<std::string_view>();
      std::string_view password_str = json_request["password"].get<std::string_view>();
      const char *username = username_str.data();
      const char *password = password_str.data();

      if (!login(username, password))
        return request::make_bad_request_response("Invalid username or password", req);

      // get user_id, generate session_id, set session_id, set session cookie
      std::string session_id = generate_session_id(0);
      int user_id = select_user_id(username, 0);
      int expires_at = 86400; // in seconds

      if (!set_session_id(session_id, user_id, username, expires_at, ip_address, 0))
        return request::make_bad_request_response("An unexpected error has occured.", req);

      return set_session_cookie(session_id);
    } else {
      return request::make_bad_request_response("Invalid request method", req);
    }
  }
};

extern "C" RequestHandler* create_user_handler() {
  pqxx::work txn(*c);
  
  /* Session Queries */
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
    
  txn.commit();
  return new UserHandler();
}