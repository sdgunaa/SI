#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace si::security {

enum class PermissionType {
  READ,    // Read files
  WRITE,   // Write/Delete files
  EXECUTE, // Run shell commands
  NETWORK, // Network access
  ENV      // Access environment variables
};

struct PermissionRequest {
  PermissionType type;
  std::string context; // e.g., file path, domain, command
  std::string reason;
};

using ApprovalCallback = std::function<bool(const PermissionRequest &)>;

class PermissionsManager {
public:
  static PermissionsManager &instance();

  /**
   * Check if an action is allowed. If not explicitly allowed,
   * it may trigger the approval callback.
   */
  bool check_permission(PermissionType type, const std::string &context);

  /**
   * Set the callback for interactive user approval
   */
  void set_approval_callback(ApprovalCallback callback);

  /**
   * Pre-approve a specific permission scope (e.g. read access to project dir)
   */
  void grant(PermissionType type, const std::string &context_pattern);

  /**
   * Revoke a permission
   */
  void revoke(PermissionType type, const std::string &context_pattern);

  // Convert enum to string for logging
  static std::string type_to_string(PermissionType type);

private:
  PermissionsManager() = default;

  // Glob/Regex match helper
  bool matches(const std::string &pattern, const std::string &value);

  std::mutex mutex_;
  ApprovalCallback approval_callback_;

  // Map PermissionType -> List of approved patterns
  std::unordered_map<PermissionType, std::vector<std::string>>
      allowed_patterns_;

  // Temporarily cache approvals for session?
  // For now, simpler list
};

} // namespace si::security
