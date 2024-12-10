#ifndef PARSER_HEADER
#define PARSER_HEADER

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace parser {
  struct Question{
    char * question;
    char ** answers;
    int answers_c;
    int answer_idx;
  };

  struct Category{
    const char * category;
    Question * questions;
    int questions_c;
  };

  nlohmann::json fetch_category(Category& cat);
  void free_question(Question& q);
  void free_category(Category& cat);
  void insert_to_category(Category& cat, Question& question, int idx);
  void create_question(Question& q, const char * question, const char** answers, int answer_c, int answer_idx);
  Category parse_category(const char * folder_dir, const char * category);
  Category * parse_categories(const char * folder_dir, const char ** category_names, int category_c);
}
#endif