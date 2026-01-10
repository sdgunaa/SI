#pragma once
#include <optional>
#include <string>

namespace si::features {

/**
 * @brief Suggested fix for a command error
 */
struct FixSuggestion {
  std::string fixed_command;
  std::string explanation;
  float confidence = 0.0f;
};

/**
 * @brief AI-powered error analysis and fix suggestions
 */
class ErrorAnalyzer {
public:
  ErrorAnalyzer();
  ~ErrorAnalyzer();

  /**
   * @brief Analyze a failed command and suggest a fix
   * @param command The original command that failed
   * @param error_output The stderr output from the command
   * @param exit_code The exit code of the failed command
   * @return Optional fix suggestion
   */
  std::optional<FixSuggestion> analyze(const std::string &command,
                                       const std::string &error_output,
                                       int exit_code);

private:
  std::string build_prompt(const std::string &command,
                           const std::string &error_output, int exit_code);
  std::optional<FixSuggestion> parse_response(const std::string &response);
};

} // namespace si::features
