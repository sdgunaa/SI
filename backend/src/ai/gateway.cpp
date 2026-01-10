#include "si/ai/gateway.hpp"
#include "si/foundation/config.hpp"
#include "si/foundation/logging.hpp"

#ifdef SI_ENABLE_OLLAMA
#include "si/ai/providers/ollama_provider.hpp"
#endif

#ifdef SI_ENABLE_OPENAI
#include "si/ai/providers/openai_provider.hpp"
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
  std::string preferred_provider = config.get_ai_provider();

  SI_LOG_INFO("Preferred AI provider: {}", preferred_provider);

  // 1. Register vLLM (via OpenAI Compatible Provider)
  // vLLM is treated as a standard OpenAI provider with a custom base URL
#ifdef SI_ENABLE_OPENAI
  {
    std::string vllm_host = config.get_vllm_host();
    if (!vllm_host.empty()) {
      // Check if full URL needs construction
      // If host is just "http://localhost:8000", we assume it points to root
      // OpenAIProvider will handle path appending if base_url is root

      try {
        // Use dummy API key for vLLM usually, but allow env override?
        // Config specific for vLLM model?
        // For now reuse openai model or add get_vllm_model()
        // Config has get_ai_model() which is generic, or specific ones.
        // Config::get_ai_model() is the generic fallback.
        // Let's assume vLLM model name is "vllm-model" or fetched from config?
        // The Config class has get_ai_model() but that might be for the active
        // one. We should probably check if there is a vLLM specific model
        // setting or just use generic. Config has get_vllm_host() only. Let's
        // use "generic" as model name for vllm if not specified, it usually
        // ignores it or user sets it via env. Actually, OpenAIProvider
        // constructor takes model name.

        std::string vllm_model = config.get_ai_model(); // Default generic model
        // Ideally config should have get_vllm_model()

        auto provider =
            std::make_unique<OpenAIProvider>("EMPTY", vllm_model, vllm_host);
        register_provider("vllm", std::move(provider));
      } catch (const std::exception &e) {
        SI_LOG_ERROR("Failed to create vLLM provider: {}", e.what());
      }
    }

    // 2. Register OpenAI (Cloud)
    std::string openai_key_env = config.get_openai_api_key_env();
    const char *api_key = getenv(openai_key_env.c_str());
    if (api_key) {
      try {
        std::string openai_model = config.get_openai_model();
        auto provider = std::make_unique<OpenAIProvider>(api_key, openai_model);
        register_provider("openai", std::move(provider));
      } catch (const std::exception &e) {
        SI_LOG_ERROR("Failed to create OpenAI provider: {}", e.what());
      }
    }
  }
#endif

#ifdef SI_ENABLE_OLLAMA
  // 3. Register Ollama
  {
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

  // Router / Fallback Selection
  if (providers_.empty()) {
    SI_LOG_WARN("No AI providers registered");
    initialized_ = true;
    return true;
  }

  // Try preferred first
  if (providers_.find(preferred_provider) != providers_.end() &&
      providers_[preferred_provider]->is_available()) {
    set_active_provider(preferred_provider);
  }
  // Else fallback priority: vllm -> ollama -> openai -> first available
  else if (providers_.count("vllm") && providers_["vllm"]->is_available()) {
    set_active_provider("vllm");
  } else if (providers_.count("ollama") &&
             providers_["ollama"]->is_available()) {
    set_active_provider("ollama");
  } else if (providers_.count("openai") &&
             providers_["openai"]->is_available()) {
    set_active_provider("openai");
  } else {
    // Just pick the first one that says it's available
    for (const auto &[name, provider] : providers_) {
      if (provider->is_available()) {
        set_active_provider(name);
        break;
      }
    }
    // If still none, just pick the first one even if not available (to avoid
    // empty)
    if (active_provider_.empty()) {
      set_active_provider(providers_.begin()->first);
    }
  }

  SI_LOG_INFO("Active AI Provider: {}", active_provider_);

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
