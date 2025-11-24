#include "foundation/config.hpp"
#include "foundation/platform.hpp"
#include <fstream>
#include <toml++/toml.h>

namespace si::foundation {

class Config::Impl {
public:
  toml::table config;
  bool loaded = false;
};

Config &Config::instance() {
  static Config inst;
  if (!inst.pimpl_) {
    inst.pimpl_ = std::make_unique<Impl>();
  }
  return inst;
}

bool Config::load(const std::filesystem::path &config_path) {
  try {
    pimpl_->config = toml::parse_file(config_path.string());
    pimpl_->loaded = true;
    return true;
  } catch (const toml::parse_error &e) {
    return false;
  }
}

bool Config::load_default() {
  // Try standard locations
  std::vector<std::filesystem::path> paths = {
      Platform::get_config_dir() / "si.conf",
      Platform::get_home_dir() / ".sirc", "/etc/si/si.conf"};

  for (const auto &path : paths) {
    if (std::filesystem::exists(path)) {
      return load(path);
    }
  }

  // Use hardcoded defaults
  pimpl_->loaded = false;
  return false;
}

std::string Config::get_shell_type() const {
  if (pimpl_->loaded) {
    return pimpl_->config["general"]["shell_type"].value_or("bash");
  }
  return Platform::get_env("SHELL", "bash");
}

bool Config::get_colors_enabled() const {
  if (pimpl_->loaded) {
    return pimpl_->config["general"]["colors"].value_or(true);
  }
  return true;
}

int Config::get_history_size() const {
  if (pimpl_->loaded) {
    return pimpl_->config["general"]["history_size"].value_or(10000);
  }
  return 10000;
}

std::string Config::get_ai_provider() const {
  if (pimpl_->loaded) {
    return pimpl_->config["ai"]["provider"].value_or("llamacpp");
  }
  return "llamacpp";
}

std::string Config::get_ai_model() const {
  if (pimpl_->loaded) {
    return pimpl_->config["ai"]["model"].value_or("codellama-7b");
  }
  return "codellama-7b";
}

float Config::get_ai_temperature() const {
  if (pimpl_->loaded) {
    return pimpl_->config["ai"]["temperature"].value_or(0.7);
  }
  return 0.7f;
}

int Config::get_ai_max_tokens() const {
  if (pimpl_->loaded) {
    return pimpl_->config["ai"]["max_tokens"].value_or(2048);
  }
  return 2048;
}

int Config::get_ai_timeout_seconds() const {
  if (pimpl_->loaded) {
    return pimpl_->config["ai"]["timeout_seconds"].value_or(30);
  }
  return 30;
}

std::string Config::get_llamacpp_model_path() const {
  if (pimpl_->loaded) {
    auto path = pimpl_->config["ai"]["llamacpp"]["model_path"].value_or(
        "~/.local/share/SI/models/codellama-7b-q4.gguf");
    return Platform::expand_path(path).string();
  }
  return (Platform::get_data_dir() / "models" / "codellama-7b-q4.gguf")
      .string();
}

int Config::get_llamacpp_gpu_layers() const {
  if (pimpl_->loaded) {
    return pimpl_->config["ai"]["llamacpp"]["gpu_layers"].value_or(32);
  }
  return 32;
}

int Config::get_llamacpp_threads() const {
  if (pimpl_->loaded) {
    return pimpl_->config["ai"]["llamacpp"]["threads"].value_or(8);
  }
  return 8;
}

std::string Config::get_ollama_host() const {
  if (pimpl_->loaded) {
    return pimpl_->config["ai"]["ollama"]["host"].value_or(
        "http://localhost:11434");
  }
  return "http://localhost:11434";
}

std::string Config::get_ollama_model() const {
  if (pimpl_->loaded) {
    return pimpl_->config["ai"]["ollama"]["model"].value_or("codellama:7b");
  }
  return "codellama:7b";
}

std::string Config::get_openai_api_key_env() const {
  if (pimpl_->loaded) {
    return pimpl_->config["ai"]["openai"]["api_key_env"].value_or(
        "OPENAI_API_KEY");
  }
  return "OPENAI_API_KEY";
}

std::string Config::get_openai_model() const {
  if (pimpl_->loaded) {
    return pimpl_->config["ai"]["openai"]["model"].value_or("gpt-4");
  }
  return "gpt-4";
}

bool Config::get_confirm_destructive() const {
  if (pimpl_->loaded) {
    return pimpl_->config["safety"]["confirm_destructive"].value_or(true);
  }
  return true;
}

bool Config::get_explain_before_run() const {
  if (pimpl_->loaded) {
    return pimpl_->config["safety"]["explain_before_run"].value_or(true);
  }
  return true;
}

bool Config::get_dry_run_available() const {
  if (pimpl_->loaded) {
    return pimpl_->config["safety"]["dry_run_available"].value_or(true);
  }
  return true;
}

std::filesystem::path Config::get_history_file() const {
  if (pimpl_->loaded) {
    auto path =
        pimpl_->config["paths"]["history_file"].value_or("~/.si_history.db");
    return Platform::expand_path(path);
  }
  return Platform::get_data_dir() / "history.db";
}

std::filesystem::path Config::get_cache_dir() const {
  if (pimpl_->loaded) {
    auto path = pimpl_->config["paths"]["cache_dir"].value_or("~/.cache/si");
    return Platform::expand_path(path);
  }
  return Platform::get_cache_dir();
}

} // namespace si::foundation
