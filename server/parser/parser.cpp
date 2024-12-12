#include "parser.hpp"

#define QUESTION_PREFIX "#Q"
#define ANSWER_PREFIX "^"

namespace parser {
  /**
   * Fetch the category struct and return/construct a JSON object.
   * @param cat Category struct to fetch.
   */
  nlohmann::json fetch_category(Category& cat) {
    nlohmann::json json_response;
    json_response["category"] = cat.category;
    auto& questions_array = json_response["questions"];

    for (int i = 0; i < cat.questions_c; i++) {
      const Question& q = cat.questions[i];
      nlohmann::json question;
      auto& answers_array = question["answers"];
      question["question"] = q.question;
      question["correct_answer"] = q.answer_idx;

      questions_array.push_back(std::move(question));
      for (int j = 0; j < q.answers_c; j++) {
        answers_array.push_back(q.answers[j]);
      }
    }
    return json_response;
  }

  /**
   * Free the memory allocated for the question and its answers.
   * @param q Question struct to free.
   */
  void free_question(Question& q) {
    free(q.question);
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
    free(const_cast<char*>(cat.category));
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
    file.open(file_loc, std::ios::binary);
  
    if (!file.is_open()) {
      std::cerr << "Category file doesn't exist!" << std::endl;
      delete[] file_loc;
      
      // file doesn't exist, return nothing
      Category no_category;
      no_category.category = "NO_CATEGORY";
      return no_category;
    }
  
    // initialize the category and questions
    Category parsed_category = {strdup(category), nullptr, 0};
    Question current_question = {nullptr, nullptr, 0, -1};
  
    char line[1024];
    char* current_answer = nullptr;
    int curr_answer_c = 0;
    char *question_text = nullptr;
  
    while (file.getline(line, sizeof(line))) {
      size_t len = std::strlen(line);
      if (len > 0 && line[len - 1] == '\r') {
        line[len - 1] = '\0';
      }
  
      if(memcmp(line, QUESTION_PREFIX, strlen(QUESTION_PREFIX)) == 0) {
        if (current_question.answers_c > 0) {
          insert_to_category(parsed_category, current_question);
          current_question = {nullptr, nullptr, 0, -1};
        }
        question_text = strdup(line + strlen(QUESTION_PREFIX) + 1);
        current_question.question = question_text;
        curr_answer_c = 0;
        free(current_answer);
        current_answer = nullptr;
      } else if (memcmp(line, ANSWER_PREFIX, strlen(ANSWER_PREFIX)) == 0) {
        free(current_answer);
        current_answer = strdup(line + strlen(ANSWER_PREFIX) + 1);
      } else if (isupper(line[0])) {
        if (current_answer) {
          char * answer = strdup(line + 2);
          insert_answer_to_question(current_question, answer);
          if (strcmp(answer, current_answer) == 0) {
            current_question.answer_idx = curr_answer_c;
          }
          curr_answer_c++;
          free(answer);
        } else {
          size_t curr_len = strlen(question_text);
          size_t add_len = strlen(line);
          char* new_text = (char*)malloc(curr_len + add_len + 2);
          strcpy(new_text, question_text);
          strcat(new_text, "\n");
          strcat(new_text, line);
          free(question_text);
          question_text = new_text;
          current_question.question = question_text;
        }
      } else if (line[0] == '\0' && current_question.answers_c > 0) {
        if (current_question.question) {
          insert_to_category(parsed_category, current_question);
          current_question = {nullptr, nullptr, 0, -1};
        }
        curr_answer_c = 0;
      }
    }
  
    if (current_question.question && current_question.answers_c > 0) {
      insert_to_category(parsed_category, current_question);
    } else {
      free_question(current_question);
    }
  
    free(current_answer);
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