#include "session.hpp"
#include <time.h>

using namespace postgres;
class SessionHandler : public RequestHandler {
  public:
  std::string get_endpoint() const override{
    return "/api/session";
  }

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req, const std::string& ip_address) {
    if (req.method() == http::verb::get) {
      /**
       * -------------- VALIDATE SESSION --------------
       */

      std::string_view session_id = request::get_session_id_from_cookie(req);
      if (session_id.empty())
        return request::make_unauthorized_response("Invalid or expired session", req);

      request::UserData user_data = request::select_user_data_from_session(session_id, 0);
      if (user_data.user_id == -1)
        return request::make_unauthorized_response("Invalid or expired session", req);

      std::optional<std::string> superuser_opt = request::parse_from_request(req, "superuser");
      nlohmann::json response_json;

      if (!superuser_opt.has_value() || (superuser_opt.has_value() && superuser_opt.value() != "true")) {
        response_json["message"] = "Session validated successfully";
        response_json["user_id"] = user_data.user_id;
        response_json["username"] = user_data.username;
        return request::make_ok_request_response(response_json.dump(4), req);
      }

      std::string * required_permissions = new std::string[1]{"superuser"};
      if (!middleware::check_permissions(request::get_user_permissions(user_data.user_id, 0), required_permissions, 1))
        return request::make_unauthorized_response("Unauthorized", req);
        
      // allow user to access admin panel
      response_json["message"] = "Session validated successfully";
      response_json["user_id"] = user_data.user_id;
      response_json["username"] = user_data.username;
      response_json["superuser"] = true;
      return request::make_ok_request_response(response_json.dump(4), req);
    } else {
      return request::make_bad_request_response("Invalid request method", req);
    }
  }
};

extern "C" RequestHandler* create_session_handler() {
  return new SessionHandler();
}