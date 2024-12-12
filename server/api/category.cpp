#include "category.hpp"

using namespace postgres;
class CategoryHandler : public RequestHandler {
  public:
    std::string get_endpoint() const override {
        return "/api/category";
    }

    /**
    * Select a category by name from the database.
    * @param category_name Name of the category to select.
    * @param verbose Whether to print messages to stdout.
    * @return ID of the category if found, 0 otherwise.
    */
    int select_category(const std::string& category_name, bool verbose) {
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

    http::response<http::string_body> handle_request(http::request<http::string_body> const& req) {
      /**
       * -------------- GET CATEGORY --------------
       */

      if (req.method() == http::verb::get) {
        auto category_opt = request::parse_category_from_request(req);
        if (!category_opt) {
            return request::make_bad_request_response("Invalid category parameters", req);
        }

        std::string category = *category_opt;
        parser::Category cat = parser::parse_category("../questions/", category.c_str());
        if (strncmp(cat.category, "NO_CATEGORY", 10) == 0) {
            return request::make_bad_request_response("Category not found", req);
        }

        return request::make_ok_request_response(parser::fetch_category(cat).dump(), req);
      } else if (req.method() == http::verb::put) {
        /**
         * -------------- PUT NEW CATEGORY --------------
         */

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
        if (!(postgres::create_category(json_request["category_name"].get<std::string>().c_str()))) {
          return request::make_bad_request_response("Category already exists", req);
        }

        response_json["message"] = "Category created successfully";
        response_json["category"] = json_request["category_name"];
        return request::make_ok_request_response(response_json.dump(4), req);
      } else if (req.method() == http::verb::delete_) {
        /**
         * -------------- DELETE CATEGORY --------------
         */

        auto category_opt = request::parse_category_from_request(req);
        if (!category_opt) {
          return request::make_bad_request_response("Invalid category parameters", req);
        }

        std::string category = *category_opt;
        nlohmann::json response_json;
        if (postgres::delete_category(category.c_str())) {
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

extern "C" RequestHandler* create_handler() {
    return new CategoryHandler();
}