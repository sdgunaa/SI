#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <si/nlohmann/json.hpp>
#include <string>

namespace si::settings {

class SettingsManager {
public:
  static SettingsManager &instance();

  // Initialize storage directory
  void ensure_storage_exists();

  // Get setting category (e.g., "appearance")
  nlohmann::json get_category(const std::string &category);

  // Save setting category
  void set_category(const std::string &category, const nlohmann::json &data);

  // Reset category to defaults (if we have them)
  void reset_category(const std::string &category);

private:
  SettingsManager();
  ~SettingsManager() = default;

  std::filesystem::path get_settings_dir() const;
  std::filesystem::path get_file_path(const std::string &category) const;

  std::mutex mutex_;
  std::map<std::string, nlohmann::json> cache_;
};

} // namespace si::settings
