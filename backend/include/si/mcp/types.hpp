#pragma once

#include <si/nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace si::mcp {

struct Tool {
  std::string name;
  std::string description;
  nlohmann::json input_schema;
};

struct ListToolsResult {
  std::vector<Tool> tools;
  std::string next_cursor;
};

struct CallToolRequest {
  std::string name;
  nlohmann::json arguments;
};

struct TextContent {
  std::string type = "text";
  std::string text;
};

struct ImageContent {
  std::string type = "image";
  std::string data; // base64
  std::string mime_type;
};

struct ToolResult {
  // Content can be text or image. Simplified to generic content list for now.
  std::vector<nlohmann::json> content;
  bool is_error = false;
};

// JSON Serialization helpers
inline void to_json(nlohmann::json &j, const Tool &t) {
  j = nlohmann::json{{"name", t.name},
                     {"description", t.description},
                     {"inputSchema", t.input_schema}};
}

inline void from_json(const nlohmann::json &j, Tool &t) {
  j.at("name").get_to(t.name);
  if (j.contains("description"))
    j.at("description").get_to(t.description);
  if (j.contains("inputSchema"))
    j.at("inputSchema").get_to(t.input_schema);
}

} // namespace si::mcp
