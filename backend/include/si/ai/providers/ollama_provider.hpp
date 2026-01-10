#pragma once

#include "si/ai/provider.hpp"
#include <memory>
#include <string>

namespace si::ai {

/**
 * Ollama Provider - Uses Ollama REST API for local LLM inference
 * Requires Ollama server running (default: http://localhost:11434)
 */
class OllamaProvider : public AIProvider {
public:
  OllamaProvider(const std::string &host, const std::string &model_name);

  ~OllamaProvider() override;

  bool initialize() override;
  bool is_available() const override;
  ModelInfo get_model_info() const override;

  CompletionResponse complete(const CompletionRequest &request) override;
  CompletionResponse stream(const CompletionRequest &request,
                            TokenCallback callback) override;

  void shutdown() override;

private:
  std::string host_;
  std::string model_name_;
  bool initialized_ = false;

  // Helper methods
  bool check_server_health();
  bool check_model_exists();
  std::string make_request(const std::string &endpoint,
                           const std::string &json_body, bool stream = false,
                           TokenCallback callback = nullptr);
};

} // namespace si::ai
