#include "si/foundation/logging.hpp"

namespace si::foundation {

Logger &Logger::instance() {
  static Logger instance;
  return instance;
}

void Logger::init(const std::string &log_file_path, Level console_level,
                  Level file_level) {
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(
      static_cast<spdlog::level::level_enum>(console_level));
  console_sink->set_pattern("[%^%l%$] %v");

  std::vector<spdlog::sink_ptr> sinks{console_sink};

  if (!log_file_path.empty()) {
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_file_path, 1024 * 1024 * 10, 3);
    file_sink->set_level(static_cast<spdlog::level::level_enum>(file_level));
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    sinks.push_back(file_sink);
  }

  logger_ = std::make_shared<spdlog::logger>("si", sinks.begin(), sinks.end());
  logger_->set_level(spdlog::level::trace);
  spdlog::register_logger(logger_);
}

std::shared_ptr<spdlog::logger> Logger::get() {
  if (!logger_) {
    init();
  }
  return logger_;
}

} // namespace si::foundation
