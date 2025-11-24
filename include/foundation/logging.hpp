#pragma once

#include <memory>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>

namespace si::foundation {

/**
 * Logging service for SI-Core
 * Provides structured logging with multiple outputs
 */
class Logger {
public:
  enum class Level { Trace, Debug, Info, Warn, Error, Critical };

  static Logger &instance();

  void init(const std::string &log_file_path = "",
            Level console_level = Level::Info, Level file_level = Level::Debug);

  std::shared_ptr<spdlog::logger> get();

  template <typename... Args>
  void trace(fmt::format_string<Args...> fmt, Args &&...args) {
    get()->trace(fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void debug(fmt::format_string<Args...> fmt, Args &&...args) {
    get()->debug(fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void info(fmt::format_string<Args...> fmt, Args &&...args) {
    get()->info(fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warn(fmt::format_string<Args...> fmt, Args &&...args) {
    get()->warn(fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void error(fmt::format_string<Args...> fmt, Args &&...args) {
    get()->error(fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void critical(fmt::format_string<Args...> fmt, Args &&...args) {
    get()->critical(fmt, std::forward<Args>(args)...);
  }

private:
  Logger() = default;
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  std::shared_ptr<spdlog::logger> logger_;
};

// Global logger macros for convenience
#define SI_LOG_TRACE(...) si::foundation::Logger::instance().trace(__VA_ARGS__)
#define SI_LOG_DEBUG(...) si::foundation::Logger::instance().debug(__VA_ARGS__)
#define SI_LOG_INFO(...) si::foundation::Logger::instance().info(__VA_ARGS__)
#define SI_LOG_WARN(...) si::foundation::Logger::instance().warn(__VA_ARGS__)
#define SI_LOG_ERROR(...) si::foundation::Logger::instance().error(__VA_ARGS__)
#define SI_LOG_CRITICAL(...)                                                   \
  si::foundation::Logger::instance().critical(__VA_ARGS__)

} // namespace si::foundation
