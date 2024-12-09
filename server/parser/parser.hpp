#ifndef PARSER_HEADER
#define PARSER_HEADER

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

  void free_question(Question& q);
  void free_category(Category& cat);
  void insert_to_category(Category& cat, Question& question, int idx);
  void create_question(Question& q, const char * question, const char** answers, int answer_c, int answer_idx);
  Category parse_category(const char * folder_dir, const char * category);
}
#endif