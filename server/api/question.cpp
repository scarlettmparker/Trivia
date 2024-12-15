#include "api.hpp"

using namespace postgres;
class QuestionHandler : public RequestHandler {
  private:
  
  /**
   * Select a question by ID from the database.
   * @param question_id ID of the question to select.
   * @param verbose Whether to print messages to stdout.
   * @return ID of the question if found, 0 otherwise.
   */
  int select_question(int question_id, int verbose) {
    try{
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("select_question", question_id);
      if (r.empty()) {
        std::cout << "Question with ID " << question_id << " not found" << std::endl;
        return 0;
      }
      verbose && std::cout << "Question with ID " << question_id << " found" << std::endl;
      return r[0][0].as<int>();
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return 0;
  }

  /**
   * Create a question in the database.
   * @param question Question to create.
   * @param answers Answers to the question.
   * @param correct_answer Index of the correct answer.
   * @param category_id ID of the category to associate the question with.
   * @param verbose Whether to print messages to stdout.
   * @return 1 if the question was created, 0 otherwise.
   */
  int create_question(const char * question, std::vector<std::string> answers,
    int correct_answer, int category_id, int verbose) {
    try {
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("create_question", question, answers, correct_answer, category_id);
      txn.commit();

      verbose && std::cout << "Successfully created question " << question << std::endl;
      return 1;
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return 0;
  }

  /**
   * Delete a question from the database.
   * @param question_id ID of the question to delete.
   * @param verbose Whether to print messages to stdout.
   * @return 1 if the question was deleted, 0 otherwise.
   */
  int delete_question(int question_id, int verbose) {
    try {
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("delete_question", question_id);
      txn.commit();

      if (!r.empty()) {
        verbose && std::cout << "Successfully deleted question with ID " << question_id << std::endl;
        return 1;
      } else {
        verbose && std::cerr << "Question with ID " << question_id << " does not exist" << std::endl;
        return 0;
      }
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return 0;
  }
  
  public:
  std::string get_endpoint() const override{
    return "/api/question";
  }

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req, const std::string& ip_address) {
    if (middleware::rate_limited(ip_address))
      return request::make_bad_request_response("Rate limited", req);

    std::string session_id = request::get_session_id_from_cookie(req);
    int user_id = request::select_user_data_from_session(session_id, 0).user_id;

    if (req.method() == http::verb::get) {
      /**
        * -------------- GET QUESTION --------------
        */

      auto question_opt = request::parse_from_request(req, "question_id");
      if (!question_opt) {
        return request::make_bad_request_response("Invalid question id parameters", req);
      }

      std::string question = *question_opt;
      int question_id;
      try {
        question_id = std::stoi(question);
      } catch (const std::invalid_argument& e) {
        return request::make_bad_request_response("Invalid question id format", req);
      } catch (const std::out_of_range& e) {
        return request::make_bad_request_response("Question id out of range", req);
      }

      nlohmann::json response_json;
      if (select_question(question_id, 0)) {
        response_json["message"] = "Question found successfully";
        response_json["question"] = question;
        return request::make_ok_request_response(response_json.dump(4), req);
      } else {
        return request::make_bad_request_response("Question not found", req);
      }
    } else if (req.method() == http::verb::put) {
      /**
        * -------------- PUT NEW QUESTION --------------
        */

      /* verify permissions*/
      std::string * required_permissions = new std::string[1]{"question.put"};
      if (!middleware::check_permissions(request::get_user_permissions(user_id, 0), required_permissions, 1))
        return request::make_unauthorized_response("Unauthorized", req);

      auto json_request = nlohmann::json::object();
      try {
        json_request = nlohmann::json::parse(req.body());
      } catch (const nlohmann::json::parse_error& e) {
        return request::make_bad_request_response("Invalid JSON request", req);
      }

      // validate request
      if (!json_request.contains("question") || !json_request.contains("answers") ||
      !json_request.contains("correct_answer") || !json_request.contains("category_id")) {
        return request::make_bad_request_response("Invalid request: Missing required fields (question | answers | correct_answer | category_id).", req);
      }
      if (!json_request["answers"].is_array() || !json_request["correct_answer"].is_number_integer())
        return request::make_bad_request_response("Invalid request: 'answers' must be an array and 'correct_answer' must be an integer.", req);

      int correct_answer = json_request["correct_answer"].get<int>();
      auto answers = json_request["answers"].get<std::vector<std::string>>();

      // validate answers
      if (correct_answer < 0 || correct_answer >= answers.size())
        return request::make_bad_request_response("Invalid request: 'correct_answer' must be an integer between 0 and the length of 'answers'.", req);

      nlohmann::json response_json;
      if (!(create_question(json_request["question"].get<std::string>().c_str(), answers, correct_answer, json_request["category_id"].get<int>(), 0)))
        return request::make_bad_request_response("An error has occured", req);

      response_json["message"] = "Question created successfully";
      response_json["question"] = json_request["question"];
      return request::make_ok_request_response(response_json.dump(4), req);
    } else if (req.method() == http::verb::delete_) {
      /**
        * -------------- DELETE QUESTION --------------
        */

      auto question_opt = request::parse_from_request(req, "question_id");
      if (!question_opt) {
        return request::make_bad_request_response("Invalid question id parameters", req);
      }

      std::string question = *question_opt;
      nlohmann::json response_json;

      int question_id;
      try {
        question_id = std::stoi(question);
      } catch (const std::invalid_argument& e) {
        return request::make_bad_request_response("Invalid question id format", req);
      } catch (const std::out_of_range& e) {
        return request::make_bad_request_response("Question id out of range", req);
      }

      if (delete_question(question_id, 0)) {
        response_json["message"] = "Question deleted successfully";
        return request::make_ok_request_response(response_json.dump(4), req);
      } else {
        return request::make_bad_request_response("Question not found", req);
      }
    } else {
      return request::make_bad_request_response("Invalid request method", req);
    }
  }
};

extern "C" RequestHandler* create_question_handler() {
  pqxx::work txn(*c);

  txn.conn().prepare("select_question", 
    "SELECT id FROM public.\"Question\" WHERE id = $1 LIMIT 1;");
  txn.conn().prepare("create_question", 
    "INSERT INTO public.\"Question\" (question, answers, correct_answer, category_id) "
    "VALUES ($1, $2, $3, $4);");
  txn.conn().prepare("delete_question", 
    "DELETE FROM public.\"Question\" WHERE id = $1 RETURNING id;");

  txn.commit();
  return new QuestionHandler();
}