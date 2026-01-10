#include "si/foundation/platform.hpp"
#include <cstdlib>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace si::foundation {

Platform::OS Platform::get_os() {
#ifdef __linux__
  return OS::Linux;
#elif __APPLE__
  return OS::MacOS;
#elif _WIN32
  return OS::Windows;
#else
  return OS::Unknown;
#endif
}

std::string Platform::get_os_name() {
  switch (get_os()) {
  case OS::Linux:
    return "Linux";
  case OS::MacOS:
    return "macOS";
  case OS::Windows:
    return "Windows";
  default:
    return "Unknown";
  }
}

bool Platform::is_posix() {
  return get_os() == OS::Linux || get_os() == OS::MacOS;
}

std::filesystem::path Platform::get_home_dir() {
  if (const char *home = std::getenv("HOME")) {
    return home;
  }
#ifdef _WIN32
  if (const char *userprofile = std::getenv("USERPROFILE")) {
    return userprofile;
  }
#endif
  return "/tmp";
}

std::filesystem::path Platform::get_config_dir() {
  if (const char *xdg_config = std::getenv("XDG_CONFIG_HOME")) {
    return std::filesystem::path(xdg_config) / "si";
  }
  return get_home_dir() / ".config" / "si";
}

std::filesystem::path Platform::get_cache_dir() {
  if (const char *xdg_cache = std::getenv("XDG_CACHE_HOME")) {
    return std::filesystem::path(xdg_cache) / "si";
  }
  return get_home_dir() / ".cache" / "si";
}

std::filesystem::path Platform::get_data_dir() {
  if (const char *xdg_data = std::getenv("XDG_DATA_HOME")) {
    return std::filesystem::path(xdg_data) / "si";
  }
  return get_home_dir() / ".local" / "share" / "si";
}

std::string Platform::get_env(const std::string &name,
                              const std::string &default_value) {
  if (const char *value = std::getenv(name.c_str())) {
    return value;
  }
  return default_value;
}

bool Platform::has_env(const std::string &name) {
  return std::getenv(name.c_str()) != nullptr;
}

void Platform::set_env(const std::string &name, const std::string &value) {
#ifdef _WIN32
  _putenv_s(name.c_str(), value.c_str());
#else
  setenv(name.c_str(), value.c_str(), 1);
#endif
}

std::filesystem::path Platform::expand_path(const std::string &path) {
  if (path.empty()) {
    return "";
  }

  std::string expanded = path;

  // Expand ~
  if (expanded[0] == '~') {
    expanded = (get_home_dir() / expanded.substr(2)).string();
  }

  // Expand environment variables ${VAR} or $VAR
  size_t pos = 0;
  while ((pos = expanded.find('$', pos)) != std::string::npos) {
    size_t end = pos + 1;
    bool braces = (end < expanded.size() && expanded[end] == '{');
    if (braces)
      end++;

    while (end < expanded.size() &&
           (std::isalnum(expanded[end]) || expanded[end] == '_')) {
      end++;
    }
    if (braces && end < expanded.size() && expanded[end] == '}') {
      end++;
    }

    std::string var_name = expanded.substr(pos + 1 + (braces ? 1 : 0),
                                           end - pos - 1 - (braces ? 2 : 0));

    std::string value = get_env(var_name);
    expanded.replace(pos, end - pos, value);
    pos += value.length();
  }

  return expanded;
}

bool Platform::is_terminal() { return isatty(STDIN_FILENO); }

bool Platform::supports_color() {
  if (!is_terminal()) {
    return false;
  }

  const char *term = std::getenv("TERM");
  if (!term) {
    return false;
  }

  std::string term_str(term);
  return term_str != "dumb";
}

std::pair<int, int> Platform::get_terminal_size() {
#ifdef TIOCGWINSZ
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
    return {w.ws_row, w.ws_col};
  }
#endif
  return {24, 80}; // Default fallback
}

} // namespace si::foundation
