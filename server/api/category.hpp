#ifndef CATEGORY_HPP
#define CATEGORY_HPP

#include <string>
#include <pqxx/pqxx>

#include "api.hpp"

class CategoryHandler : public RequestHandler {
public:
    std::string get_endpoint() const override;
    int select_category(const std::string& category_name, int verbose);
    int select_category_by_id(int category_id);
    int create_category(const char* category_name, int verbose);
    int delete_category(const char* category_name, int verbose);
    http::response<http::string_body> handle_request(http::request<http::string_body> const& req);
};

#endif