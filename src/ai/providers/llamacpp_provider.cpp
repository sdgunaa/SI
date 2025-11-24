#include "ai/providers/llamacpp_provider.hpp"
#include "foundation/logging.hpp"
#include "llama.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

namespace si::ai {

// Helper to convert std::string to std::vector<llama_token>
static std::vector<llama_token>
tokenize(llama_context *ctx, const std::string &text, bool add_bos) {
  const llama_model *model = llama_get_model(ctx);
  const llama_vocab *vocab = llama_model_get_vocab(model);
  int n_tokens = text.length() + add_bos;
  std::vector<llama_token> tokens(n_tokens);
  n_tokens = llama_tokenize(vocab, text.c_str(), text.length(), tokens.data(),
                            tokens.size(), add_bos, false);
  if (n_tokens < 0) {
    tokens.resize(-n_tokens);
    n_tokens = llama_tokenize(vocab, text.c_str(), text.length(), tokens.data(),
                              tokens.size(), add_bos, false);
  }
  tokens.resize(n_tokens);
  return tokens;
}

LlamaCppProvider::LlamaCppProvider(const std::string &model_path,
                                   int n_gpu_layers, int n_threads, int n_ctx)
    : model_path_(model_path), n_gpu_layers_(n_gpu_layers),
      n_threads_(n_threads > 0 ? n_threads
                               : std::thread::hardware_concurrency()),
      n_ctx_(n_ctx) {

  // Initialize backend
  llama_backend_init();
}

LlamaCppProvider::~LlamaCppProvider() {
  shutdown();
  llama_backend_free();
}

bool LlamaCppProvider::initialize() {
  if (initialized_)
    return true;

  SI_LOG_INFO("Initializing llama.cpp provider...");
  SI_LOG_INFO("  Model: {}", model_path_);
  SI_LOG_INFO("  GPU Layers: {}", n_gpu_layers_);
  SI_LOG_INFO("  Threads: {}", n_threads_);

  // Load model
  llama_model_params model_params = llama_model_default_params();
  model_params.n_gpu_layers = n_gpu_layers_;

  model_ = llama_model_load_from_file(model_path_.c_str(), model_params);
  if (!model_) {
    SI_LOG_ERROR("Failed to load model from {}", model_path_);
    return false;
  }

  // Create context
  llama_context_params ctx_params = llama_context_default_params();
  ctx_params.n_ctx = n_ctx_;
  ctx_params.n_threads = n_threads_;
  ctx_params.n_threads_batch = n_threads_;

  ctx_ = llama_init_from_model(model_, ctx_params);
  if (!ctx_) {
    SI_LOG_ERROR("Failed to create llama context");
    llama_model_free(model_);
    model_ = nullptr;
    return false;
  }

  initialized_ = true;
  SI_LOG_INFO("llama.cpp provider initialized successfully");
  return true;
}

bool LlamaCppProvider::is_available() const {
  return initialized_ && model_ && ctx_;
}

ModelInfo LlamaCppProvider::get_model_info() const {
  ModelInfo info;
  info.name = model_path_; // Or extract filename
  info.type = "gguf";
  info.context_window = n_ctx_;
  info.supports_gpu = (n_gpu_layers_ > 0);
  info.loaded = initialized_;
  return info;
}

void LlamaCppProvider::create_sampler(float temperature) {
  if (sampler_) {
    llama_sampler_free(sampler_);
  }

  sampler_ = llama_sampler_chain_init(llama_sampler_chain_default_params());
  llama_sampler_chain_add(sampler_, llama_sampler_init_temp(temperature));
  llama_sampler_chain_add(sampler_, llama_sampler_init_dist(1234)); // Seed
}

CompletionResponse
LlamaCppProvider::complete(const CompletionRequest &request) {
  return stream(request, nullptr);
}

CompletionResponse LlamaCppProvider::stream(const CompletionRequest &request,
                                            TokenCallback callback) {
  CompletionResponse response;
  auto start_time = std::chrono::high_resolution_clock::now();

  if (!is_available()) {
    response.success = false;
    response.error_message = "Provider not initialized";
    return response;
  }

  // Tokenize prompt
  std::vector<llama_token> tokens_list = tokenize(ctx_, request.prompt, true);

  // Check context size
  if ((int)tokens_list.size() > n_ctx_ - 4) {
    response.success = false;
    response.error_message = "Prompt too long for context window";
    return response;
  }

  // Clear KV cache
  llama_memory_clear(llama_get_memory(ctx_), true);

  // Create sampler
  create_sampler(request.temperature);

  // Evaluate prompt
  if (llama_decode(
          ctx_, llama_batch_get_one(tokens_list.data(), tokens_list.size()))) {
    response.success = false;
    response.error_message = "Failed to decode prompt";
    return response;
  }

  int n_cur = tokens_list.size();
  int n_decode = 0;
  std::string result_text = "";
  const llama_vocab *vocab = llama_model_get_vocab(model_);

  while (n_decode < request.max_tokens) {
    // Sample next token
    llama_token new_token_id = llama_sampler_sample(sampler_, ctx_, -1);

    // Check for EOS
    if (llama_vocab_is_eog(vocab, new_token_id)) {
      break;
    }

    // Convert to string
    char buf[256];
    int n =
        llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
    if (n < 0) {
      response.success = false;
      response.error_message = "Failed to convert token to piece";
      break;
    }
    std::string piece(buf, n);

    // Append to result
    result_text += piece;
    if (callback) {
      callback(piece);
    }

    // Prepare next batch
    if (llama_decode(ctx_, llama_batch_get_one(&new_token_id, 1))) {
      response.success = false;
      response.error_message = "Failed to decode token";
      break;
    }

    n_cur++;
    n_decode++;
  }

  response.content = result_text;
  response.tokens_used = n_decode;
  response.success = true;

  auto end_time = std::chrono::high_resolution_clock::now();
  response.latency_ms =
      std::chrono::duration<float, std::milli>(end_time - start_time).count();

  return response;
}

void LlamaCppProvider::shutdown() {
  if (sampler_) {
    llama_sampler_free(sampler_);
    sampler_ = nullptr;
  }
  if (ctx_) {
    llama_free(ctx_);
    ctx_ = nullptr;
  }
  if (model_) {
    llama_model_free(model_);
    model_ = nullptr;
  }
  initialized_ = false;
}

} // namespace si::ai
