#pragma once

#include <si/nlohmann/json.hpp>
#include <string>
#include <vector>

namespace si::ai {

/**
 * Builds context for AI prompts from current shell state
 */
class ContextBuilder {
public:
  static ContextBuilder &instance();

  // Gather full context as JSON
  nlohmann::json build_context();

  // Get system prompt for command generation
  std::string get_command_generation_prompt();

  // Get system prompt for error analysis
  std::string get_error_analysis_prompt();

  // Set current working directory
  void set_cwd(const std::string &cwd);

  // Set current session for history access
  void set_session_id(const std::string &session_id);

private:
  ContextBuilder() = default;

  std::string current_cwd_ = ".";
  std::string session_id_ = "default";
};

} // namespace si::ai
