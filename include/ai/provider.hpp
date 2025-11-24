#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace si::ai {

/**
 * Completion request to LLM provider
 */
struct CompletionRequest {
  std::string prompt;
  float temperature = 0.7f;
  int max_tokens = 2048;
  int timeout_seconds = 30;
  std::vector<std::string> stop_sequences;
};

/**
 * Response from LLM provider
 */
struct CompletionResponse {
  std::string content;
  int tokens_used = 0;
  float latency_ms = 0.0f;
  bool success = false;
  std::string error_message;
};

/**
 * Token callback for streaming responses
 */
using TokenCallback = std::function<void(const std::string &token)>;

/**
 * Model information
 */
struct ModelInfo {
  std::string name;
  std::string type; // "gguf", "ollama", "openai"
  int context_window = 2048;
  bool supports_gpu = false;
  bool loaded = false;
};

/**
 * Abstract AI Provider interface
 * All LLM backends must implement this interface
 */
class AIProvider {
public:
  virtual ~AIProvider() = default;

  /**
   * Initialize the provider with configuration
   */
  virtual bool initialize() = 0;

  /**
   * Check if provider is ready for inference
   */
  virtual bool is_available() const = 0;

  /**
   * Get model information
   */
  virtual ModelInfo get_model_info() const = 0;

  /**
   * Synchronous completion
   */
  virtual CompletionResponse complete(const CompletionRequest &request) = 0;

  /**
   * Streaming completion with token callback
   */
  virtual CompletionResponse stream(const CompletionRequest &request,
                                    TokenCallback callback) = 0;

  /**
   * Shutdown and cleanup
   */
  virtual void shutdown() = 0;

protected:
  AIProvider() = default;
};

} // namespace si::ai
