#include <cstdlib>
#include <iostream>
#include <si/settings/settings_manager.hpp>

namespace si::settings {

SettingsManager &SettingsManager::instance() {
  static SettingsManager instance;
  return instance;
}

SettingsManager::SettingsManager() { ensure_storage_exists(); }

std::filesystem::path SettingsManager::get_settings_dir() const {
#ifdef _WIN32
  const char *appdata = std::getenv("APPDATA");
  std::filesystem::path base = appdata ? appdata : "C:/ProgramData";
  return base / "ShellAI" / "settings";
#elif __APPLE__
  const char *home = std::getenv("HOME");
  std::filesystem::path base = home ? home : "/tmp";
  return base / "Library" / "Application Support" / "ShellAI" / "settings";
#else
  const char *home = std::getenv("HOME");
  std::filesystem::path base = home ? home : "/tmp";
  return base / ".local" / "share" / "shellai" / "settings";
#endif
}

void SettingsManager::ensure_storage_exists() {
  std::filesystem::create_directories(get_settings_dir());
}

std::filesystem::path
SettingsManager::get_file_path(const std::string &category) const {
  return get_settings_dir() / (category + ".json");
}

nlohmann::json SettingsManager::get_category(const std::string &category) {
  std::lock_guard<std::mutex> lock(mutex_);

  // Check cache first
  if (cache_.count(category)) {
    return cache_[category];
  }

  auto path = get_file_path(category);
  if (std::filesystem::exists(path)) {
    try {
      std::ifstream f(path);
      nlohmann::json data;
      f >> data;
      cache_[category] = data;
      return data;
    } catch (const std::exception &e) {
      std::cerr << "Error reading settings file " << path << ": " << e.what()
                << std::endl;
    }
  }

  return nlohmann::json::object();
}

void SettingsManager::set_category(const std::string &category,
                                   const nlohmann::json &data) {
  std::lock_guard<std::mutex> lock(mutex_);

  // Update cache
  cache_[category] = data;

  // Save to disk
  auto path = get_file_path(category);
  try {
    std::ofstream f(path);
    f << data.dump(4);
  } catch (const std::exception &e) {
    std::cerr << "Error writing settings file " << path << ": " << e.what()
              << std::endl;
  }
}

void SettingsManager::reset_category(const std::string &category) {
  std::lock_guard<std::mutex> lock(mutex_);
  cache_.erase(category);

  auto path = get_file_path(category);
  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }
}

} // namespace si::settings
