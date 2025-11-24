#pragma once

#include <optional>
#include <string>
#include <vector>

namespace si::features {

struct CommandResult {
  std::string command;
  std::string explanation;
  float confidence;
  bool is_safe;
};

class CommandInterpreter {
public:
  CommandInterpreter();
  ~CommandInterpreter();

  /**
   * Interpret natural language input and generate shell command
   */
  std::optional<CommandResult> interpret(const std::string &input);

  /**
   * Explain a shell command in natural language
   */
  std::string explain(const std::string &command);

private:
  std::string build_prompt(const std::string &input);
  CommandResult parse_response(const std::string &response);
  bool is_destructive(const std::string &command);
};

} // namespace si::features
