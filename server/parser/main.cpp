#include <iostream>
#include "parser.hpp"

int main() {
  const char * folder_dir = "../questions/";
  const char * category_name = "animals";

  parser::Category category;
  category = parser::parse_category(folder_dir, category_name);

  std::cout << "Category: " << category.category << std::endl;
  for (int i = 0; i < category.questions_c; i++) {
    parser::Question question = category.questions[i];
    std::cout << "Question: " << question.question << std::endl;
    std::cout << "Answer Index: " << question.answer_idx << std::endl;

    for (int i = 0; i < question.answers_c; i++) {
      std::cout << i << ". " << question.answers[i] << std::endl;
    }
    std::cout << std::endl;
  }

  parser::free_category(category);
  return 0;
}