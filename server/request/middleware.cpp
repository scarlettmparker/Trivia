#include "middleware.hpp"

namespace middleware {
  /* rate limiting */
  int MAX_REQUESTS_PER_SECOND = 5;
  std::mutex rate_limit_mutex;
  std::unordered_map<std::string, RateLimitData> rate_limit_cache;

  /**
   * Check if a user has the required permissions.
   * This function checks if the user has all of the required permissions to access a resource.
   *
   * @param user_permissions Permissions the user has.
   * @param required_permissions Permissions required to access the resource.
   * @param num_permissions Number of permissions required.
   * @return 1 if the user has the required permissions, 0 otherwise.
   */
  int check_permissions(request::UserPermissions permissions, std::string * required_permissions, int num_permissions) {
    std::unordered_set<std::string> permission_set;
    for (int i = 0; i < permissions.permission_count; i++) {
      if (permissions.permissions[i].permission_name == "*")
        return 1;
      permission_set.insert(permissions.permissions[i].permission_name);
    }
    
    for (int i = 0; i < num_permissions; i++) {
      if (permission_set.find(required_permissions[i]) == permission_set.end())
        return 0;
    }
    return 1;
  }

  /**
   * Check if a user is being rate limited.
   * This works by checking the number of requests made by the user in the last second.
   * If the user has made more than MAX_REQUESTS_PER_SECOND requests, they are rate limited.
   *
   * @param ip_address IP address of the user to check.
   * @return 1 if the user is rate limited, 0 otherwise.
   */
  int rate_limited(const std::string& ip_address) {
    std::lock_guard<std::mutex> guard(rate_limit_mutex);
    auto now = std::chrono::system_clock::now();
    auto& data = rate_limit_cache[ip_address];

    if (data.last_request.time_since_epoch().count() == 0 ||
      std::chrono::duration_cast<std::chrono::seconds>(now - data.last_request).count() > 1) {
      data.request_count = 1;
      data.last_request = now;
      return 0;
    }

    if (std::chrono::duration_cast<std::chrono::seconds>(now - data.last_request).count() == 0) {
      if (data.request_count > MAX_REQUESTS_PER_SECOND) {
        return 1;
      }
      data.request_count++;
    } else {
      data.request_count = 1;
      data.last_request = now;
    }
    return 0;
  }
}