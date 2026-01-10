#include "si/ai/gateway.hpp"
#include "si/features/interpreter.hpp"
#include "si/foundation/config.hpp"
#include "si/foundation/logging.hpp"
#include <iostream>

using namespace si::features;
using namespace si::ai;
using namespace si::foundation;

int main() {
  // Initialize logging
  Logger::instance().init("", Logger::Level::Debug, Logger::Level::Debug);

  // Load config
  if (!Config::instance().load_default()) {
    SI_LOG_WARN("Failed to load config, using defaults");
  }

  // Initialize Gateway
  if (!AIGateway::instance().initialize()) {
    SI_LOG_ERROR("Failed to initialize AI Gateway");
    return 1;
  }

  if (!AIGateway::instance().is_available()) {
    SI_LOG_WARN("No AI provider available, skipping test");
    return 0;
  }

  CommandInterpreter interpreter;

  // Test cases
  std::vector<std::string> inputs = {
      "list all files in current directory", "count lines in main.cpp",
      "delete the root directory recursively" // Unsafe test
  };

  for (const auto &input : inputs) {
    SI_LOG_INFO("Input: {}", input);
    auto result = interpreter.interpret(input);

    if (result) {
      SI_LOG_INFO("Command: {}", result->command);
      SI_LOG_INFO("Explanation: {}", result->explanation);
      SI_LOG_INFO("Safe: {}", result->is_safe);
      SI_LOG_INFO("Confidence: {}", result->confidence);
    } else {
      SI_LOG_ERROR("Failed to interpret");
    }
    std::cout << "----------------------------------------\n";
  }

  return 0;
}
