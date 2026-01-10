#include "si/ai/providers/ollama_provider.hpp"
#include "si/foundation/config.hpp"
#include "si/foundation/logging.hpp"
#include <chrono>
#include <iostream>
#include <thread>

using namespace si::ai;
using namespace si::foundation;

int main() {
  // Initialize logging
  Logger::instance().init("", Logger::Level::Debug, Logger::Level::Debug);

  SI_LOG_INFO("Testing Ollama Provider...");

  // Initialize Config (required for logging/platform)
  Config::instance();

  // Create provider
  // Assuming default host and deepseek-r1:1.5b model
  OllamaProvider provider("http://localhost:11434", "deepseek-r1:1.5b");

  if (!provider.initialize()) {
    SI_LOG_ERROR("Failed to initialize Ollama provider");
    SI_LOG_ERROR(
        "Make sure Ollama is running and 'deepseek-r1:1.5b' is pulled");
    return 1;
  }

  SI_LOG_INFO("Provider initialized successfully");

  // Test Model Info
  auto info = provider.get_model_info();
  SI_LOG_INFO("Model Info:");
  SI_LOG_INFO("  Name: {}", info.name);
  SI_LOG_INFO("  Type: {}", info.type);
  SI_LOG_INFO("  Context: {}", info.context_window);

  // Test Completion
  CompletionRequest req;
  req.prompt =
      "Generate a bash command to list all PDF files in the current directory.";
  req.max_tokens = 100;
  req.temperature = 0.7f;

  SI_LOG_INFO("Testing completion...");
  SI_LOG_INFO("Prompt: {}", req.prompt);

  auto resp = provider.complete(req);

  if (resp.success) {
    SI_LOG_INFO("Response received ({:.2f}ms):", resp.latency_ms);
    std::cout << "----------------------------------------" << std::endl;
    std::cout << resp.content << std::endl;
    std::cout << "----------------------------------------" << std::endl;
  } else {
    SI_LOG_ERROR("Completion failed: {}", resp.error_message);
    return 1;
  }

  // Test Streaming
  SI_LOG_INFO("Testing streaming...");
  req.prompt = "Explain what 'ls -la' does in one sentence.";

  std::cout << "Stream: ";
  auto stream_resp = provider.stream(
      req, [](const std::string &token) { std::cout << token << std::flush; });
  std::cout << std::endl;

  if (stream_resp.success) {
    SI_LOG_INFO("Streaming finished successfully ({:.2f}ms)",
                stream_resp.latency_ms);
  } else {
    SI_LOG_ERROR("Streaming failed: {}", stream_resp.error_message);
    return 1;
  }

  return 0;
}
