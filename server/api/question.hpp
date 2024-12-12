#ifndef QUESTION_HPP
#define QUESTION_HPP

#include <string>
#include <vector>
#include <pqxx/pqxx>

#include "api.hpp"
#include "category.hpp"

class QuestionHandler : public RequestHandler {
public:
    std::string get_endpoint() const override;
    int select_question(int question_id);
    int create_question(const char* question, std::vector<std::string> answers, int correct_answer, int category_id);
    int delete_question(int question_id);
    http::response<http::string_body> handle_request(http::request<http::string_body> const& req);
};

#endif