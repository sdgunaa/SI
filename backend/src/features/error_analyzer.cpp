#include "si/features/error_analyzer.hpp"
#include "si/ai/gateway.hpp"
#include "si/foundation/logging.hpp"
#include <si/nlohmann/json.hpp>
#include <regex>

namespace si::features {

ErrorAnalyzer::ErrorAnalyzer() {}
ErrorAnalyzer::~ErrorAnalyzer() {}

std::string ErrorAnalyzer::build_prompt(const std::string &command,
                                        const std::string &error_output,
                                        int exit_code) {
  return "You are a shell expert. A command failed. Suggest a fix.\n"
         "Output ONLY valid JSON.\n\n"
         "Example:\n"
         "Command: gti status\n"
         "Error: command not found: gti\n"
         "JSON: {\"fixed_command\": \"git status\", \"explanation\": \"Typo: "
         "gti -> git\", \"confidence\": 0.95}\n\n"
         "Command: " +
         command +
         "\n"
         "Error: " +
         error_output +
         "\n"
         "Exit code: " +
         std::to_string(exit_code) +
         "\n"
         "JSON:";
}

std::optional<FixSuggestion>
ErrorAnalyzer::parse_response(const std::string &response) {
  FixSuggestion fix;

  // Strip <think> blocks if present
  std::string cleaned = response;
  std::regex think_regex("<think>[\\s\\S]*?</think>");
  cleaned = std::regex_replace(cleaned, think_regex, "");

  // Find JSON
  size_t start = cleaned.find('{');
  size_t end = cleaned.rfind('}');

  if (start == std::string::npos || end == std::string::npos || end <= start) {
    return std::nullopt;
  }

  try {
    auto json = nlohmann::json::parse(cleaned.substr(start, end - start + 1));
    fix.fixed_command = json.value("fixed_command", "");
    fix.explanation = json.value("explanation", "");
    fix.confidence = json.value("confidence", 0.0f);

    if (fix.fixed_command.empty())
      return std::nullopt;
    return fix;
  } catch (...) {
    return std::nullopt;
  }
}

std::optional<FixSuggestion>
ErrorAnalyzer::analyze(const std::string &command,
                       const std::string &error_output, int exit_code) {
  auto &gateway = ai::AIGateway::instance();
  if (!gateway.is_available())
    return std::nullopt;

  auto prompt = build_prompt(command, error_output, exit_code);

  ai::CompletionRequest req;
  req.prompt = prompt;
  req.max_tokens = 512;
  req.temperature = 0.1f;

  auto response = gateway.complete(req);
  if (!response.success)
    return std::nullopt;

  return parse_response(response.content);
}

} // namespace si::features
