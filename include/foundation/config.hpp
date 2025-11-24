#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace si::foundation {

/**
 * Configuration manager for SI-Core
 * Loads and manages TOML configuration from ~/.config/si/si.conf
 */
class Config {
public:
  static Config &instance();

  // Load configuration from file
  bool load(const std::filesystem::path &config_path);
  bool load_default();

  // General settings
  std::string get_shell_type() const;
  bool get_colors_enabled() const;
  int get_history_size() const;

  // AI settings
  std::string get_ai_provider() const;
  std::string get_ai_model() const;
  float get_ai_temperature() const;
  int get_ai_max_tokens() const;
  int get_ai_timeout_seconds() const;

  // llama.cpp settings
  std::string get_llamacpp_model_path() const;
  int get_llamacpp_gpu_layers() const;
  int get_llamacpp_threads() const;

  // Ollama settings
  std::string get_ollama_host() const;
  std::string get_ollama_model() const;

  // OpenAI settings
  std::string get_openai_api_key_env() const;
  std::string get_openai_model() const;

  // Safety settings
  bool get_confirm_destructive() const;
  bool get_explain_before_run() const;
  bool get_dry_run_available() const;

  // Path settings
  std::filesystem::path get_history_file() const;
  std::filesystem::path get_cache_dir() const;

  // Raw access for custom keys
  template <typename T> std::optional<T> get(const std::string &key) const;

private:
  Config() = default;
  Config(const Config &) = delete;
  Config &operator=(const Config &) = delete;

  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

} // namespace si::foundation
