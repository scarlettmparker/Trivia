#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include "parser.hpp"

#define QUESTION_PREFIX "#Q"
#define ANSWER_PREFIX "^"

namespace parser {
  /**
   * Free the memory allocated for the question and its answers.
   * @param q Question struct to free.
   */
  void free_question(Question& q) {
    delete[] q.question;
    for (int i = 0; i < q.answers_c; i++) {
      free(q.answers[i]);
    }
    free(q.answers);
  }

  /**
   * Free the memory allocated for the category and its questions.
   * @param cat Category struct to free.
   */
  void free_category(Category& cat) {
    for (int i = 0; i < cat.questions_c; i++) {
      free_question(cat.questions[i]);
    }
    free(cat.questions);
  }

  /**
   * Insert a question to a category, reallocating the memory for the questions.
   * It does this by copying the question to a new memory location and then
   * inserting it to the questions array of the category.
   *
   * @param cat Category struct to insert the question to.
   * @param question Question to insert to the category.
   */
  void insert_to_category(Category& cat, Question& question) {
    cat.questions = (Question *) realloc(cat.questions, sizeof(Question) * (cat.questions_c + 1));
    cat.questions[cat.questions_c] = question;
    cat.questions_c++;
  }

  /**
   * Insert an answer to a question, reallocating the memory for the answers.
   * It does this by copying the answer to a new memory location and then
   * inserting it to the answers array of the question.
   *
   * @param q Question struct to insert the answer to.
   * @param answer Answer to insert to the question.
   */
  void insert_answer_to_question(Question& q, const char* answer) {
    size_t len = strlen(answer);
    q.answers = (char**) realloc(q.answers, (q.answers_c + 1) * sizeof(char*));
    q.answers[q.answers_c] = (char*) malloc(len + 1);

    memcpy(q.answers[q.answers_c], answer, len);
    q.answers[q.answers_c][len] = '\0';
    q.answers_c++;
  }

  /**
   * Parse the category file and return the category struct.
   * This reads the category file line by line and parses the questions and answers.
   *
   * @param folder_dir Directory of the folder containing the category file.
   * @param category Name of the category file.
   */
  Category parse_category(const char * folder_dir, const char * category) {
    size_t length = std::strlen(folder_dir) + std::strlen(category) + 1;
    char * file_loc = new char[length];

    std::strcpy(file_loc, folder_dir);
    std::strcat(file_loc, category);
    std::ifstream file;
    file.open(file_loc);

    if (!file.is_open()) {
      std::cerr << "Category file doesn't exist!" << std::endl;
      delete[] file_loc;
      
      // file doesn't exist, return nothing
      Category no_category;
      no_category.category = "NO_CATEGORY";
      file.close();
      return no_category;
    }

    // initialize the category and questions
    Category parsed_category;
    parsed_category.category = category;
    parsed_category.questions = nullptr;
    parsed_category.questions_c = 0;

    char line[1024];
    int curr_answer_c = 0;

    while (file.getline(line, sizeof(line))) {
      size_t len = std::strlen(line);
      if (len > 0 && line[len - 1] == '\r') {
        line[len - 1] = '\0';
      }

      if (strncmp(line, QUESTION_PREFIX, strlen(QUESTION_PREFIX)) == 0) {
        const char* question_text = line + 3;

        Question question;
        question.question = new char[strlen(question_text) + 1];
        question.answers = nullptr;
        question.answers_c = 0;
        question.answer_idx = -1;

        std::strcpy(question.question, question_text);
        curr_answer_c = 0;

        // read and find the answers
        char* current_answer = nullptr;
        while(file.getline(line, sizeof(line))) {
          len = std::strlen(line);
          if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
          if (len == 0) break;
          
          if (strncmp(line, ANSWER_PREFIX, strlen(ANSWER_PREFIX)) == 0) {
            free(current_answer);
            current_answer = strdup(line + 2);
          } else if (std::isupper(line[0])) {
            // line starts with an uppercase letter, it's an answer
            curr_answer_c++;
            char* answer = strdup(line + 2);

            insert_answer_to_question(question, answer);
            if (current_answer && strcmp(answer, current_answer) == 0) {
              question.answer_idx = curr_answer_c - 1;
            }
            free(answer);
          }
        }

        if (current_answer) free(current_answer);
        question.answers_c > 0 ? insert_to_category(parsed_category, question)
          : free_question(question);
      }
    }

    file.close();
    delete[] file_loc;
    return parsed_category;
  }

  /**
   * Parse the categories and return the category structs.
   * @param folder_dir Directory of the folder containing the category files.
   * @param categories Array of category names.
   * @param categories_c Number of categories.
   */
  Category * parse_categories(const char * folder_dir, const char ** categories, int categories_c) {
    Category * parsed_categories = (Category *) malloc(sizeof(Category) * categories_c);
    for (int i = 0; i < categories_c; i++) {
      parsed_categories[i] = parse_category(folder_dir, categories[i]);
    }
    return parsed_categories;
  }
}