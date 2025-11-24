#pragma once

#include <filesystem>
#include <string>

namespace si::foundation {

/**
 * Platform abstraction utilities
 */
class Platform {
public:
  // Platform detection
  enum class OS { Linux, MacOS, Windows, Unknown };

  static OS get_os();
  static std::string get_os_name();
  static bool is_posix();

  // Path utilities
  static std::filesystem::path get_home_dir();
  static std::filesystem::path get_config_dir();
  static std::filesystem::path get_cache_dir();
  static std::filesystem::path get_data_dir();

  // Environment variables
  static std::string get_env(const std::string &name,
                             const std::string &default_value = "");
  static bool has_env(const std::string &name);
  static void set_env(const std::string &name, const std::string &value);

  // Expand paths with ~ and environment variables
  static std::filesystem::path expand_path(const std::string &path);

  // Terminal capabilities
  static bool is_terminal();
  static bool supports_color();
  static std::pair<int, int> get_terminal_size();
};

} // namespace si::foundation
