#include "si/security/permissions.hpp"
#include "si/foundation/logging.hpp"
#include <fnmatch.h> // POSIX glob matching
#include <iostream>

namespace si::security {

PermissionsManager &PermissionsManager::instance() {
  static PermissionsManager inst;
  return inst;
}

void PermissionsManager::set_approval_callback(ApprovalCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  approval_callback_ = callback;
}

void PermissionsManager::grant(PermissionType type,
                               const std::string &context_pattern) {
  std::lock_guard<std::mutex> lock(mutex_);
  allowed_patterns_[type].push_back(context_pattern);
  SI_LOG_INFO("Permission GRANTED: {} for {}", type_to_string(type),
              context_pattern);
}

void PermissionsManager::revoke(PermissionType type,
                                const std::string &context_pattern) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto &patterns = allowed_patterns_[type];
  // Remove exact matches (simple implementation)
  // A better one would handle logic better
  auto it = std::remove(patterns.begin(), patterns.end(), context_pattern);
  if (it != patterns.end()) {
    patterns.erase(it, patterns.end());
    SI_LOG_INFO("Permission REVOKED: {} for {}", type_to_string(type),
                context_pattern);
  }
}

bool PermissionsManager::check_permission(PermissionType type,
                                          const std::string &context) {
  std::lock_guard<std::mutex> lock(mutex_);

  // 1. Check pre-approved patterns
  if (allowed_patterns_.count(type)) {
    for (const auto &pattern : allowed_patterns_[type]) {
      if (matches(pattern, context)) {
        return true;
      }
    }
  }

  // 2. Ask User (if callback exists)
  if (approval_callback_) {
    PermissionRequest req{type, context, "Agent tool requested access"};
    // Release lock during callback to prevent deadlocks if callback calls back
    // into PM But here we need to be careful. Generally callbacks shouldn't
    // re-enter. Unlocking is safer.
    mutex_.unlock();
    bool approved = approval_callback_(req);
    mutex_.lock();

    if (approved) {
      // Optional: auto-grant for this session?
      // allowed_patterns_[type].push_back(context);
      // Let's decide NOT to auto-grant permanently to be safe, or maybe just
      // this instance.
      return true;
    }
  }

  SI_LOG_WARN("Permission DENIED: {} for {}", type_to_string(type), context);
  return false;
}

bool PermissionsManager::matches(const std::string &pattern,
                                 const std::string &value) {
  // 0 on match, FNM_NOMATCH on no match
  return fnmatch(pattern.c_str(), value.c_str(), 0) == 0;
}

std::string PermissionsManager::type_to_string(PermissionType type) {
  switch (type) {
  case PermissionType::READ:
    return "READ";
  case PermissionType::WRITE:
    return "WRITE";
  case PermissionType::EXECUTE:
    return "EXECUTE";
  case PermissionType::NETWORK:
    return "NETWORK";
  case PermissionType::ENV:
    return "ENV";
  default:
    return "UNKNOWN";
  }
}

} // namespace si::security
