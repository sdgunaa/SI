#pragma once

#include <si/nlohmann/json.hpp>
#include <string>
#include <vector>

namespace si::shell {

struct WorkflowArgument {
  std::string name;
  std::string description;
  std::string default_value;
};

struct Workflow {
  std::string id;
  std::string name;
  std::string description;
  std::string command_template; // e.g., "git commit -m \"{{message}}\""
  std::vector<WorkflowArgument> arguments;
  std::vector<std::string> tags;
};

// JSON Serialization
inline void to_json(nlohmann::json &j, const WorkflowArgument &a) {
  j = nlohmann::json{{"name", a.name},
                     {"description", a.description},
                     {"default_value", a.default_value}};
}
inline void from_json(const nlohmann::json &j, WorkflowArgument &a) {
  j.at("name").get_to(a.name);
  j.at("description").get_to(a.description);
  if (j.contains("default_value"))
    j.at("default_value").get_to(a.default_value);
}

inline void to_json(nlohmann::json &j, const Workflow &w) {
  j = nlohmann::json{{"id", w.id},
                     {"name", w.name},
                     {"description", w.description},
                     {"command_template", w.command_template},
                     {"arguments", w.arguments},
                     {"tags", w.tags}};
}
inline void from_json(const nlohmann::json &j, Workflow &w) {
  j.at("id").get_to(w.id);
  j.at("name").get_to(w.name);
  j.at("description").get_to(w.description);
  j.at("command_template").get_to(w.command_template);
  if (j.contains("arguments"))
    j.at("arguments").get_to(w.arguments);
  if (j.contains("tags"))
    j.at("tags").get_to(w.tags);
}

} // namespace si::shell
