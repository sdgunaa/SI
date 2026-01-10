#include "si/ai/gateway.hpp"
#include "si/foundation/config.hpp"
#include "si/foundation/logging.hpp"
#include <chrono>
#include <iostream>
#include <thread>

using namespace si::ai;
using namespace si::foundation;

int main(int argc, char **argv) {
  // Initialize logging
  Logger::instance().init("", Logger::Level::Debug, Logger::Level::Debug);

  SI_LOG_INFO("Testing AI Gateway...");

  // Load configuration
  if (Config::instance().load_default()) {
    SI_LOG_INFO("Loaded configuration");
  } else {
    SI_LOG_WARN("Using default configuration (config file not found)");
  }

  // Initialize Gateway
  auto &gateway = AIGateway::instance();
  if (!gateway.initialize()) {
    SI_LOG_ERROR("Failed to initialize gateway");
    return 1;
  }

  // List providers
  auto providers = gateway.list_providers();
  SI_LOG_INFO("Available providers: {}", providers.size());
  for (const auto &p : providers) {
    SI_LOG_INFO("  - {}", p);
  }

  if (providers.empty()) {
    SI_LOG_WARN(
        "No providers available. Please configure a provider in si.conf");
    // We can't test much else without a provider
    return 0;
  }

  // Test completion
  CompletionRequest req;
  req.prompt = "question: Hello, i am Guna, kindly explain what is AI? \n "
               "answer: Hi Guna, AI is ";
  req.max_tokens = 50;

  SI_LOG_INFO("Testing completion...");
  auto resp = gateway.complete(req);

  if (resp.success) {
    SI_LOG_INFO("Response: {}", resp.content);
    SI_LOG_INFO("Stats: {} tokens, {:.2f}ms", resp.tokens_used,
                resp.latency_ms);
  } else {
    SI_LOG_ERROR("Completion failed: {}", resp.error_message);
  }

  // Test streaming
  SI_LOG_INFO("Testing streaming...");
  std::cout << "Stream output: ";

  auto stream_resp = gateway.stream(
      req, [](const std::string &token) { std::cout << token << std::flush; });
  std::cout << "\n";

  if (stream_resp.success) {
    SI_LOG_INFO("Streaming finished successfully");
  } else {
    SI_LOG_ERROR("Streaming failed: {}", stream_resp.error_message);
  }

  gateway.shutdown();
  return 0;
}
