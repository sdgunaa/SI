#include "si/foundation/config.hpp"
#include "si/foundation/platform.hpp"
#include <fstream>
#include <iostream>
#include <toml++/toml.h>

namespace si::foundation {

class Config::Impl {
public:
  toml::table config;
  bool loaded = false;

  // Helper to merge tables
  void merge(toml::table &target, const toml::table &source) {
    for (const auto &[key, value] : source) {
      if (value.is_table() && target.contains(key) && target[key].is_table()) {
        merge(*target[key].as_table(), *value.as_table());
      } else {
        target.insert_or_assign(key, value);
      }
    }
  }
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

bool Config::load_merge(const std::filesystem::path &config_path) {
  try {
    auto new_conf = toml::parse_file(config_path.string());
    pimpl_->merge(pimpl_->config, new_conf);
    pimpl_->loaded = true;
    return true;
  } catch (const toml::parse_error &e) {
    return false;
  }
}

bool Config::load_default() {
  bool any_loaded = false;

  // 1. System/Global config
  std::vector<std::filesystem::path> global_paths = {
      "/etc/si/si.conf", Platform::get_config_dir() / "si.conf",
      Platform::get_home_dir() / ".sirc"};

  for (const auto &path : global_paths) {
    if (std::filesystem::exists(path)) {
      if (load_merge(path))
        any_loaded = true;
    }
  }

  // 2. Per-project config (walk up from CWD)
  std::filesystem::path current = std::filesystem::current_path();
  std::filesystem::path root = current.root_path();

  // Potential project config files to look for
  std::vector<std::string> project_configs = {".si/config.toml", ".si.toml"};

  while (current != root) {
    for (const auto &cfg : project_configs) {
      std::filesystem::path p = current / cfg;
      if (std::filesystem::exists(p)) {
        load_merge(p);
        any_loaded = true;
        goto project_found; // Stop at first project scope found
      }
    }
    current = current.parent_path();
  }
project_found:

  pimpl_->loaded = any_loaded;
  return any_loaded;
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
    return pimpl_->config["ai"]["provider"].value_or("vllm");
  }
  return "vllm";
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

std::string Config::get_vllm_host() const {
  if (pimpl_->loaded) {
    return pimpl_->config["ai"]["vllm"]["host"].value_or(
        "http://localhost:8000");
  }
  return "http://localhost:8000";
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

template <typename T>
std::optional<T> Config::get(const std::string &key) const {
  if (!pimpl_->loaded)
    return std::nullopt;
  // Simple key access for now, assuming 1 level. For dotted need recursive
  // split. toml++ supports node_view paths?
  return pimpl_->config[key].value<T>();
}

} // namespace si::foundation
