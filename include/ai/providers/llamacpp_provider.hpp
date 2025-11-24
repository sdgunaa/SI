#pragma once

#include "ai/provider.hpp"
#include <memory>
#include <string>

// Forward declaration of llama.cpp types
struct llama_model;
struct llama_context;
struct llama_sampler;

namespace si::ai {

/**
 * LlamaCpp Provider - Local LLM inference with GPU support
 * Uses llama.cpp for efficient local model execution
 */
class LlamaCppProvider : public AIProvider {
public:
  LlamaCppProvider(const std::string &model_path, int n_gpu_layers = 0,
                   int n_threads = 8, int n_ctx = 2048);

  ~LlamaCppProvider() override;

  bool initialize() override;
  bool is_available() const override;
  ModelInfo get_model_info() const override;

  CompletionResponse complete(const CompletionRequest &request) override;
  CompletionResponse stream(const CompletionRequest &request,
                            TokenCallback callback) override;

  void shutdown() override;

private:
  std::string model_path_;
  int n_gpu_layers_;
  int n_threads_;
  int n_ctx_;

  llama_model *model_ = nullptr;
  llama_context *ctx_ = nullptr;
  llama_sampler *sampler_ = nullptr;

  bool initialized_ = false;

  // Helper methods
  std::string generate_impl(const std::string &prompt, int max_tokens,
                            float temperature, TokenCallback callback);

  void create_sampler(float temperature);
};

} // namespace si::ai
