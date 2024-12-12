#include "api.hpp"

using namespace postgres;
class QuestionHandler : public RequestHandler {
  public:
  std::string get_endpoint() const override{
    return "/api/question";
  }

  int select_question(int question_id, int verbose=0) {
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
      std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Unknown error while executing query" << std::endl;
    }
  }

  int create_question(const char * question, std::vector<std::string> answers,
    int correct_answer, int category_id, int verbose=0) {
    try {
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("create_question", question, answers, correct_answer, category_id);
      txn.commit();

      verbose && std::cout << "Successfully created question " << question << std::endl;
      return 1;
    } catch (const std::exception &e) {
      std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Unknown error while executing query" << std::endl;
    }
    return 0;
  }

  int delete_question(int question_id, int verbose=0) {
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
      std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Unknown error while executing query" << std::endl;
    }
    return 0;
  }

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req) {
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

      if (delete_question(question_id), 0) {
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
  return new QuestionHandler();
}