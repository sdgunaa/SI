#pragma once

#include "si/ai/provider.hpp"
#include <string>

namespace si::ai {

/**
 * OpenAI Provider - Uses OpenAI API for cloud inference
 * Requires OPENAI_API_KEY environment variable
 */
class OpenAIProvider : public AIProvider {
public:
  OpenAIProvider(const std::string &api_key,
                 const std::string &model_name = "gpt-4",
                 const std::string &base_url = "https://api.openai.com");

  ~OpenAIProvider() override;

  bool initialize() override;
  bool is_available() const override;
  ModelInfo get_model_info() const override;

  CompletionResponse complete(const CompletionRequest &request) override;
  CompletionResponse stream(const CompletionRequest &request,
                            TokenCallback callback) override;

  void shutdown() override;

private:
  std::string api_key_;
  std::string model_name_;
  std::string base_url_;
  bool initialized_ = false;

  // Helper methods
  std::string make_request(const std::string &endpoint,
                           const std::string &json_body, bool stream = false,
                           TokenCallback callback = nullptr);
};

} // namespace si::ai
