#pragma once

#include "foundation/config.hpp"
#include "provider.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace si::ai {

/**
 * AI Gateway - manages multiple providers and routes requests
 */
class AIGateway {
public:
  static AIGateway &instance();

  /**
   * Initialize gateway with configuration
   */
  bool initialize();

  /**
   * Register a provider
   */
  void register_provider(const std::string &name,
                         std::unique_ptr<AIProvider> provider);

  /**
   * Set active provider by name
   */
  bool set_active_provider(const std::string &name);

  /**
   * Get current active provider name
   */
  std::string get_active_provider_name() const;

  /**
   * Check if any provider is available
   */
  bool is_available() const;

  /**
   * Get model info from active provider
   */
  ModelInfo get_model_info() const;

  /**
   * Completion using active provider
   */
  CompletionResponse complete(const CompletionRequest &request);

  /**
   * Streaming completion using active provider
   */
  CompletionResponse stream(const CompletionRequest &request,
                            TokenCallback callback);

  /**
   * List all registered providers
   */
  std::vector<std::string> list_providers() const;

  /**
   * Shutdown all providers
   */
  void shutdown();

private:
  AIGateway() = default;
  AIGateway(const AIGateway &) = delete;
  AIGateway &operator=(const AIGateway &) = delete;

  std::unordered_map<std::string, std::unique_ptr<AIProvider>> providers_;
  std::string active_provider_;
  bool initialized_ = false;
};

} // namespace si::ai
