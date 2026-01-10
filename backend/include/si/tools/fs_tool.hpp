#pragma once

#include "si/tools/tool_base.hpp"
#include <filesystem>

namespace si::tools {

class FsTool : public ToolBase {
public:
  si::mcp::Tool get_definition() const override {
    return {
        "fs_read",
        "Read file contents",
        {{"type", "object"},
         {"properties",
          {{"path", {{"type", "string"}, {"description", "Path to file"}}}}},
         {"required", {"path"}}}};
  }

  si::mcp::ToolResult execute(const nlohmann::json &args) override;
};

} // namespace si::tools
