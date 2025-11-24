#include "ai/gateway.hpp"
#include "foundation/config.hpp"
#include "foundation/logging.hpp"

#ifdef SI_HAS_LLAMACPP
#include "ai/providers/llamacpp_provider.hpp"
#endif

#ifdef SI_ENABLE_OLLAMA
#include "ai/providers/ollama_provider.hpp"
#endif

#ifdef SI_ENABLE_OPENAI
#include "ai/providers/openai_provider.hpp"
#endif

namespace si::ai {

AIGateway &AIGateway::instance() {
  static AIGateway instance;
  return instance;
}

bool AIGateway::initialize() {
  if (initialized_) {
    return true;
  }

  SI_LOG_INFO("Initializing AI Gateway...");

  auto &config = foundation::Config::instance();
  std::string provider_type = config.get_ai_provider();

  SI_LOG_INFO("Configured AI provider: {}", provider_type);

#ifdef SI_HAS_LLAMACPP
  if (provider_type == "llamacpp" || provider_type == "auto") {
    std::string model_path = config.get_llamacpp_model_path();
    int gpu_layers = config.get_llamacpp_gpu_layers();
    int threads = config.get_llamacpp_threads();

    if (model_path.front() == '~') {
      const char *home = getenv("HOME");
      if (home) {
        model_path.replace(0, 1, home);
      }
    }

    try {
      auto provider =
          std::make_unique<LlamaCppProvider>(model_path, gpu_layers, threads);
      register_provider("llamacpp", std::move(provider));
    } catch (const std::exception &e) {
      SI_LOG_ERROR("Failed to create LlamaCpp provider: {}", e.what());
    }
  }
#endif

#ifdef SI_ENABLE_OLLAMA
  if (provider_type == "ollama" || provider_type == "auto") {
    std::string host = config.get_ollama_host();
    std::string model = config.get_ollama_model();

    try {
      auto provider = std::make_unique<OllamaProvider>(host, model);
      register_provider("ollama", std::move(provider));
    } catch (const std::exception &e) {
      SI_LOG_ERROR("Failed to create Ollama provider: {}", e.what());
    }
  }
#endif

#ifdef SI_ENABLE_OPENAI
  if (provider_type == "openai" || provider_type == "auto") {
    std::string api_key_env = config.get_openai_api_key_env();
    std::string model = config.get_openai_model();

    const char *api_key = getenv(api_key_env.c_str());
    if (api_key) {
      try {
        auto provider = std::make_unique<OpenAIProvider>(api_key, model);
        register_provider("openai", std::move(provider));
      } catch (const std::exception &e) {
        SI_LOG_ERROR("Failed to create OpenAI provider: {}", e.what());
      }
    } else {
      if (provider_type == "openai") {
        SI_LOG_ERROR("OpenAI API key not found in environment variable: {}",
                     api_key_env);
      }
    }
  }
#endif

  if (!providers_.empty()) {
    if (providers_.find(provider_type) != providers_.end()) {
      set_active_provider(provider_type);
    } else {
      set_active_provider(providers_.begin()->first);
    }
  } else {
    SI_LOG_WARN("No AI providers available");
  }

  initialized_ = true;
  return true;
}

void AIGateway::register_provider(const std::string &name,
                                  std::unique_ptr<AIProvider> provider) {
  SI_LOG_INFO("Registering AI provider: {}", name);

  if (provider->initialize()) {
    providers_[name] = std::move(provider);

    if (active_provider_.empty()) {
      active_provider_ = name;
      SI_LOG_INFO("Set {} as active provider", name);
    }
  } else {
    SI_LOG_ERROR("Failed to initialize provider: {}", name);
  }
}

bool AIGateway::set_active_provider(const std::string &name) {
  auto it = providers_.find(name);
  if (it == providers_.end()) {
    SI_LOG_ERROR("Provider not found: {}", name);
    return false;
  }

  if (!it->second->is_available()) {
    SI_LOG_ERROR("Provider not available: {}", name);
    return false;
  }

  active_provider_ = name;
  SI_LOG_INFO("Switched to provider: {}", name);
  return true;
}

std::string AIGateway::get_active_provider_name() const {
  return active_provider_;
}

bool AIGateway::is_available() const {
  if (active_provider_.empty()) {
    return false;
  }

  auto it = providers_.find(active_provider_);
  if (it == providers_.end()) {
    return false;
  }

  return it->second->is_available();
}

ModelInfo AIGateway::get_model_info() const {
  if (!is_available()) {
    return ModelInfo{};
  }

  auto it = providers_.find(active_provider_);
  return it->second->get_model_info();
}

CompletionResponse AIGateway::complete(const CompletionRequest &request) {
  if (!is_available()) {
    CompletionResponse response;
    response.success = false;
    response.error_message = "No AI provider available";
    SI_LOG_ERROR(response.error_message);
    return response;
  }

  auto it = providers_.find(active_provider_);
  SI_LOG_DEBUG("Sending completion request to {}", active_provider_);

  auto response = it->second->complete(request);

  if (response.success) {
    SI_LOG_DEBUG("Completion successful: {} tokens, {:.2f}ms",
                 response.tokens_used, response.latency_ms);
  } else {
    SI_LOG_ERROR("Completion failed: {}", response.error_message);
  }

  return response;
}

CompletionResponse AIGateway::stream(const CompletionRequest &request,
                                     TokenCallback callback) {
  if (!is_available()) {
    CompletionResponse response;
    response.success = false;
    response.error_message = "No AI provider available";
    SI_LOG_ERROR(response.error_message);
    return response;
  }

  auto it = providers_.find(active_provider_);
  SI_LOG_DEBUG("Sending streaming request to {}", active_provider_);

  return it->second->stream(request, callback);
}

std::vector<std::string> AIGateway::list_providers() const {
  std::vector<std::string> names;
  names.reserve(providers_.size());

  for (const auto &[name, _] : providers_) {
    names.push_back(name);
  }

  return names;
}

void AIGateway::shutdown() {
  SI_LOG_INFO("Shutting down AI Gateway...");

  for (auto &[name, provider] : providers_) {
    SI_LOG_DEBUG("Shutting down provider: {}", name);
    provider->shutdown();
  }

  providers_.clear();
  active_provider_.clear();
  initialized_ = false;
}

} // namespace si::ai
