#include "api.hpp"

using namespace postgres;
class CategoryHandler : public RequestHandler {
  private:
  
  struct Category {
    std::string category_name;
    int id;
  };

  struct CategoryData {
    Category * categories;
    int count;
  };

  /**
  * Select a category by name from the database.
  * @param category_name Name of the category to select.
  * @param verbose Whether to print messages to stdout.
  * @return ID of the category if found, 0 otherwise.
  */
  int select_category(const std::string& category_name, int verbose) {
    try {
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("select_category", category_name);
      if (r.empty()) {
        verbose && std::cout << "Category " << category_name << " not found" << std::endl;
        return 0;
      }
      verbose && std::cout << "Category " << category_name << " found" << std::endl;
      return r[0][0].as<int>();
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    }
    return 0;
  }

  /**
   * Get a list of category data (names and IDs) from the database.
   * This is used to display on the admin panel for category management.
   * 
   * @param limit Number of categories to fetch.
   * @param offset Offset to start fetching categories from.
   * @param verbose Whether to print messages to stdout.
   * @return Category Data names and IDs if found, nullptr otherwise.
   */
  CategoryData get_category_data(int limit, int offset, int verbose) {
    try {
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("select_category_names_pagington", limit, offset * limit);
      int count = r.size();
      if (count == 0)
          return {nullptr, 0};

      std::unique_ptr<Category[]> categories;
      try {
        categories = std::make_unique<Category[]>(count);
      } catch (const std::bad_alloc& e) {
        std::cerr << "Memory allocation failed: " << e.what() << std::endl;
        return {nullptr, 0};
      }

      for (int i = 0; i < count; ++i) {
        categories[i].category_name = r[i][0].as<std::string>();
        categories[i].id = r[i][1].as<int>();
      }

      return {categories.release(), count};
    } catch (const std::exception &e) {
      std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Unknown error while executing query" << std::endl;
    }

    return {nullptr, 0};
  }
  
  /**
  * Create a category in the database.
  * @param category_name Name of the category to create.
  * @param verbose Whether to print messages to stdout.
  * @return 1 if the category was created, 0 otherwise.
  */
  int create_category(const char * category_name, int verbose) {
    try {
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("create_category", category_name);
      txn.commit();

      if (!r.empty()) {
        verbose && std::cout << "Successfully created category " << category_name << std::endl;
        return 1;
      } else {
        verbose && std::cerr << "Category " << category_name << " already exists" << std::endl;
        return 0;
      }
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return 0;
  }

  /**
  * Delete a category from the database.
  * @param category_name Name of the category to delete.
  * @param verbose Whether to print messages to stdout.
  * @return 1 if the category was deleted, 0 otherwise.
  */
  int delete_category(const char * category_name, int verbose) {
    if (c == nullptr) {
      verbose && std::cerr << "Database connection is not initialized" << std::endl;
      return 0;
    }

    try {
      pqxx::work txn(*c);
      pqxx::result r = txn.exec_prepared("delete_category", category_name);
      txn.commit();

      if (!r.empty()) {
        verbose && std::cout << "Successfully deleted category with name " << category_name << std::endl;
        return 1;
      } else {
        verbose && std::cerr << "Category with name " << category_name << " does not exist" << std::endl;
        return 0;
      }
    } catch (const std::exception &e) {
      verbose && std::cerr << "Error executing query: " << e.what() << std::endl;
    } catch (...) {
      verbose && std::cerr << "Unknown error while executing query" << std::endl;
    }
    return 0;
  }

  /**
   * Handle a request to fetch a single category. This function is
   * used to fetch a single category by name.
   *
   * @param req HTTP request object.
   * @param user_id ID of the user making the request.
   * @param category_name Name of the category to fetch.
   * @return HTTP response object.
   */
  http::response<http::string_body> handle_single_category(const http::request<http::string_body>& req,
    int user_id, const std::string& category_name) {
    std::string* required_permissions = new std::string[1]{"category.admin"};
    if (!middleware::check_permissions(request::get_user_permissions(user_id, 0), required_permissions, 1))
      return request::make_unauthorized_response("Unauthorized", req);
    delete[] required_permissions;

    parser::Category cat = parser::parse_category("../questions/", category_name.c_str());
    if (strncmp(cat.category, "NO_CATEGORY", 10) == 0)
      return request::make_bad_request_response("Category not found", req);
    return request::make_ok_request_response(parser::fetch_category(cat).dump(), req);
  }

  /**
   * Handle a request to list categories. This function is used to
   * list all categories for use in the admin panel.
   * 
   * @param req HTTP request object.
   * @param user_id ID of the user making the request.
   * @param page_size Number of categories to fetch.
   * @param offset Offset to start fetching categories from.
   * @return HTTP response object.
   */
  http::response<http::string_body> handle_category_list(const http::request<http::string_body>& req, int user_id,
    const std::string& page_size, const std::optional<std::string>& offset) {
    std::string* required_permissions = new std::string[2]{"superuser", "category.admin"};
    if (!middleware::check_permissions(request::get_user_permissions(user_id, 0), required_permissions, 2))
      return request::make_unauthorized_response("Unauthorized", req);
    delete[] required_permissions;

    int pages_int, offset_int = 0;
    if (offset.has_value() && !validate_pagination_params(page_size, offset.value(), pages_int, offset_int)) {
      return request::make_bad_request_response("Invalid request: 'page_size|page' invalid.", req);
    }

    if (!offset.has_value()) {
      try {
        pages_int = std::stoi(page_size);
      } catch (...) {
        return request::make_bad_request_response("Invalid request: 'page_size' must be an integer.", req);
      }
    }

    nlohmann::json response_json;
    CategoryData category_data = get_category_data(pages_int, offset_int, 0);
    
    if (category_data.count == 0) {
      response_json["message"] = "No categories found";
      response_json["categories"] = nlohmann::json::array();
      return request::make_ok_request_response(response_json.dump(4), req);
    }

    response_json["message"] = "Categories fetched successfully";
    response_json["categories"] = nlohmann::json::array();

    for (int i = 0; i < category_data.count; ++i) {
      response_json["categories"].push_back({
        {"category_name", category_data.categories[i].category_name},
        {"id", category_data.categories[i].id}
      });
    }

    delete[] category_data.categories;
    return request::make_ok_request_response(response_json.dump(4), req);
  }

  /**
   * Validate pagination parameters. This function checks if the page size and offset are valid integers.
   * @param page_size Page size to validate.
   * @param offset Offset to validate.
   * @param pages_int Reference to store the page size as an integer.
   * @param offset_int Reference to store the offset as an integer.
   * @return 1 if the parameters are valid, 0 otherwise.
   */
  int validate_pagination_params(const std::string& page_size, const std::string& offset, int& pages_int, int& offset_int) {
    try {
      pages_int = std::stoi(page_size);
      offset_int = std::stoi(offset);
      return 1;
    } catch (const std::invalid_argument& e) {
      return 0;
    } catch (const std::out_of_range& e) {
      return 0;
    }
  }

  public:
  std::string get_endpoint() const override {
    return "/api/category";
  }

  http::response<http::string_body> handle_request(http::request<http::string_body> const& req, const std::string& ip_address) {  
    if (middleware::rate_limited(ip_address))
      return request::make_too_many_requests_response("Too many requests", req);

    std::string_view session_id = request::get_session_id_from_cookie(req);
    int user_id = request::select_user_data_from_session(session_id, 0).user_id;
    if (req.method() == http::verb::get) {
      std::optional<std::string> category_opt = request::parse_from_request(req, "category_name");
      if (category_opt.has_value()) {
        return handle_single_category(req, user_id, category_opt.value());
      }

      std::optional<std::string> superuser_opt = request::parse_from_request(req, "superuser");
      std::optional<std::string> pages_opt = request::parse_from_request(req, "page_size");
      std::optional<std::string> offset_opt = request::parse_from_request(req, "page");

      if (!superuser_opt.has_value() || superuser_opt.value() != "true")
        return request::make_bad_request_response("Endpoint not implemented", req);
      if (!pages_opt.has_value())
        return request::make_bad_request_response("Invalid request: Missing required field (page_size).", req);
      return handle_category_list(req, user_id, pages_opt.value(), offset_opt);
    } else if (req.method() == http::verb::put) {
      /**
        * -------------- PUT NEW CATEGORY --------------
        */

      std::string * required_permissions = new std::string[1]{"category.put"};
      if (!middleware::check_permissions(request::get_user_permissions(user_id, 0), required_permissions, 1))
        return request::make_unauthorized_response("Unauthorized", req);

      auto json_request = nlohmann::json::object();
      try {
        json_request = nlohmann::json::parse(req.body());
      } catch (const nlohmann::json::parse_error& e) {
        return request::make_bad_request_response("Invalid JSON request", req);
      }

      if (!json_request.contains("category_name")) {
        return request::make_bad_request_response("Invalid request: Missing required field (category_name).", req);
      }

      nlohmann::json response_json;
      if (!(create_category(json_request["category_name"].get<std::string>().c_str(), 0))) {
        return request::make_bad_request_response("Category already exists", req);
      }

      response_json["message"] = "Category created successfully";
      response_json["category"] = json_request["category_name"];
      return request::make_ok_request_response(response_json.dump(4), req);
    } else if (req.method() == http::verb::delete_) {
      /**
        * -------------- DELETE CATEGORY --------------
        */

      std::string * required_permissions = new std::string[1]{"category.delete"};
      if (!middleware::check_permissions(request::get_user_permissions(user_id, 0), required_permissions, 1))
        return request::make_unauthorized_response("Unauthorized", req);

      auto category_opt = request::parse_from_request(req, "category_name");
      if (!category_opt) {
        return request::make_bad_request_response("Invalid category parameters", req);
      }

      std::string category = *category_opt;
      nlohmann::json response_json;
      if (delete_category(category.c_str(), 1)) {
        response_json["message"] = "Category deleted successfully";
        response_json["category_name"] = category;
        return request::make_ok_request_response(response_json.dump(4), req);
      } else {
        return request::make_bad_request_response("Category not found", req);
      }
    } else {
      return request::make_bad_request_response("Invalid method", req);
    }
  }
};

extern "C" RequestHandler* create_category_handler() {
  pqxx::work txn(*c);

  txn.conn().prepare("select_category_names_pagington",
    "SELECT category_name, id FROM public.\"Category\" ORDER BY category_name ASC LIMIT $1 OFFSET $2;");
  txn.conn().prepare("create_category", 
    "INSERT INTO public.\"Category\" (category_name) VALUES ($1) "
    "ON CONFLICT (category_name) DO NOTHING RETURNING id;");
  txn.conn().prepare("delete_category", 
    "DELETE FROM public.\"Category\" WHERE category_name = $1 RETURNING id;");
    
  txn.commit();
  return new CategoryHandler();
}