#pragma once

#include "si/mcp/types.hpp"
#include "si/security/permissions.hpp"
#include <string>

namespace si::tools {

class ToolBase {
public:
  virtual ~ToolBase() = default;

  // Get tool definition (name, description, schema)
  virtual si::mcp::Tool get_definition() const = 0;

  // Execute tool
  virtual si::mcp::ToolResult execute(const nlohmann::json &args) = 0;

protected:
  // Helper to check permission
  bool check_permission(si::security::PermissionType type,
                        const std::string &context) {
    return si::security::PermissionsManager::instance().check_permission(
        type, context);
  }
};

} // namespace si::tools
