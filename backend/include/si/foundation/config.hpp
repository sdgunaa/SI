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
  // Load and merge configuration from file (overlays existing)
  // Load configuration from file (overwrite)
  bool load(const std::filesystem::path &config_path);
  // Load defaults using standard paths
  bool load_default();

  // Load and merge configuration from file (overlays existing)
  bool load_merge(const std::filesystem::path &config_path);

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

  // Ollama settings
  std::string get_ollama_host() const;
  std::string get_ollama_model() const;

  // vLLM settings
  std::string get_vllm_host() const;

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
