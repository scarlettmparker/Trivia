#ifndef MIDDLEWARE_HPP
#define MIDDLEWARE_HPP

#include <iostream>
#include <string>
#include <unordered_set>
#include "request.hpp"

namespace middleware {
  struct RateLimitData {
    int request_count;
    std::chrono::system_clock::time_point window_start;
    std::chrono::system_clock::time_point last_request;
  };

  // rate limiting
  extern int MAX_REQUESTS_PER_SECOND;
  extern std::mutex rate_limit_mutex;
  extern std::unordered_map<std::string, RateLimitData> rate_limit_cache;

  int check_permissions(request::UserPermissions user_permissions, std::string * required_permissions, int num_permissions);
  int rate_limited(const std::string& ip_address);
}

#endif