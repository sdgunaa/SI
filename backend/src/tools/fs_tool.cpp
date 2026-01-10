#include "si/tools/fs_tool.hpp"
#include "si/foundation/logging.hpp"
#include "si/foundation/platform.hpp" // For path expansion
#include <fstream>
#include <sstream>

namespace si::tools {

si::mcp::ToolResult FsTool::execute(const nlohmann::json &args) {
  si::mcp::ToolResult result;

  if (!args.contains("path")) {
    result.is_error = true;
    result.content.push_back(
        {{"type", "text"}, {"text", "Missing 'path' argument"}});
    return result;
  }

  std::string raw_path = args["path"].get<std::string>();
  auto path = si::foundation::Platform::expand_path(raw_path);

  // Permission Check
  if (!check_permission(si::security::PermissionType::READ, path.string())) {
    result.is_error = true;
    result.content.push_back({{"type", "text"}, {"text", "Permission denied"}});
    return result;
  }

  if (!std::filesystem::exists(path)) {
    result.is_error = true;
    result.content.push_back({{"type", "text"}, {"text", "File not found"}});
    return result;
  }

  try {
    std::ifstream f(path);
    std::stringstream buffer;
    buffer << f.rdbuf();
    result.content.push_back({{"type", "text"}, {"text", buffer.str()}});
  } catch (const std::exception &e) {
    result.is_error = true;
    result.content.push_back(
        {{"type", "text"}, {"text", std::string("Read error: ") + e.what()}});
  }

  return result;
}

} // namespace si::tools
