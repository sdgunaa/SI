#include "si/features/interpreter.hpp"
#include "si/ai/gateway.hpp"
#include "si/foundation/logging.hpp"
#include <iostream>
#include <si/nlohmann/json.hpp>
#include <regex>

using json = nlohmann::json;

namespace si::features {

CommandInterpreter::CommandInterpreter() {}
CommandInterpreter::~CommandInterpreter() {}

std::string CommandInterpreter::build_prompt(const std::string &input) {
  return "You are a command line expert. Translate the request into a bash "
         "command.\n"
         "Output MUST be a valid JSON object. Do not output any other text.\n\n"
         "Example 1:\n"
         "Request: list files\n"
         "JSON Output: {\"command\": \"ls -la\", \"explanation\": \"Lists all "
         "files\", \"safe\": true, \"confidence\": 1.0}\n\n"
         "Example 2:\n"
         "Request: delete everything\n"
         "JSON Output: {\"command\": \"rm -rf .\", \"explanation\": "
         "\"Recursively deletes all files\", \"safe\": false, \"confidence\": "
         "0.9}\n\n"
         "Example 3:\n"
         "Request: count lines in main.cpp\n"
         "JSON Output: {\"command\": \"wc -l main.cpp\", \"explanation\": "
         "\"Counts lines in file\", \"safe\": true, \"confidence\": 0.95}\n\n"
         "Request: " +
         input +
         "\n"
         "JSON Output:";
}

std::optional<CommandResult>
CommandInterpreter::interpret(const std::string &input) {
  auto &gateway = ai::AIGateway::instance();
  if (!gateway.is_available()) {
    SI_LOG_ERROR("AI Gateway not available");
    return std::nullopt;
  }

  std::string prompt = build_prompt(input);

  ai::CompletionRequest req;
  req.prompt = prompt;
  req.max_tokens = 1024;
  req.temperature = 0.1f;

  SI_LOG_DEBUG("Sending interpretation request...");
  auto response = gateway.complete(req);

  if (!response.success) {
    SI_LOG_ERROR("AI request failed: {}", response.error_message);
    return std::nullopt;
  }

  return parse_response(response.content);
}

CommandResult CommandInterpreter::parse_response(const std::string &response) {
  CommandResult result;
  result.confidence = 0.0f;
  result.is_safe = false;

  std::string clean_response = response;

  std::regex think_regex("<think>[\\s\\S]*?</think>");
  clean_response = std::regex_replace(clean_response, think_regex, "");

  try {
    size_t start = clean_response.find('{');
    size_t end = clean_response.rfind('}');

    if (start != std::string::npos && end != std::string::npos && end > start) {
      std::string json_str = clean_response.substr(start, end - start + 1);
      auto j = json::parse(json_str);

      if (j.contains("command"))
        result.command = j["command"].get<std::string>();
      if (j.contains("explanation"))
        result.explanation = j["explanation"].get<std::string>();
      if (j.contains("safe"))
        result.is_safe = j["safe"].get<bool>();
      if (j.contains("confidence"))
        result.confidence = j["confidence"].get<float>();
    } else {
      SI_LOG_WARN("No JSON found in response");
      SI_LOG_DEBUG("Response content: {}", clean_response);

      std::regex code_regex("```(?:bash|sh)?\\s*(.*?)\\s*```");
      std::smatch match;
      if (std::regex_search(clean_response, match, code_regex)) {
        result.command = match[1].str();
        result.explanation = "Extracted from code block";
        result.confidence = 0.5f;
      }
    }
  } catch (const std::exception &e) {
    SI_LOG_ERROR("Failed to parse JSON response: {}", e.what());
    SI_LOG_DEBUG("Raw response: {}", response);
  }

  // Override safety with heuristic check
  if (result.is_safe && is_destructive(result.command)) {
    SI_LOG_WARN("AI marked destructive command as safe. Overriding.");
    result.is_safe = false;
  }

  return result;
}

bool CommandInterpreter::is_destructive(const std::string &command) {
  // Simple heuristic for destructive commands
  // This could be expanded with a more sophisticated AI check or a curated list
  if (command.find("rm -rf") != std::string::npos ||
      command.find("mv /") != std::string::npos ||
      command.find("dd if=/dev/zero") != std::string::npos ||
      command.find("mkfs") != std::string::npos) {
    return true;
  }
  return false;
}

std::string CommandInterpreter::explain(const std::string &command) {
  auto &gateway = ai::AIGateway::instance();
  if (!gateway.is_available())
    return "AI unavailable";

  std::string prompt = "Explain the following bash command briefly:\n" +
                       command + "\n\nExplanation:";

  ai::CompletionRequest req;
  req.prompt = prompt;
  req.max_tokens = 100;

  auto response = gateway.complete(req);
  return response.success ? response.content : "Failed to explain";
}

} // namespace si::features
