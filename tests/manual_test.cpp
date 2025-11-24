#include "si.hpp"
#include <cassert>
#include <iostream>

using namespace si;
using namespace si::foundation;

void test_platform() {
  std::cout << "Testing Platform utilities...\n";

  // Test OS detection
  auto os = Platform::get_os();
  auto os_name = Platform::get_os_name();
  std::cout << "  OS: " << os_name << "\n";
  assert(!os_name.empty());

  // Test paths
  auto home = Platform::get_home_dir();
  std::cout << "  Home: " << home << "\n";
  assert(!home.empty());

  auto config_dir = Platform::get_config_dir();
  std::cout << "  Config dir: " << config_dir << "\n";

  auto cache_dir = Platform::get_cache_dir();
  std::cout << "  Cache dir: " << cache_dir << "\n";

  // Test environment
  Platform::set_env("SI_TEST", "hello");
  assert(Platform::has_env("SI_TEST"));
  assert(Platform::get_env("SI_TEST") == "hello");
  std::cout << "  Env variables: OK\n";

  // Test path expansion
  auto expanded = Platform::expand_path("~/test");
  std::cout << "  Path expansion: " << expanded << "\n";
  assert(expanded.string().find('~') == std::string::npos);

  // Test terminal
  bool is_tty = Platform::is_terminal();
  std::cout << "  Is terminal: " << (is_tty ? "yes" : "no") << "\n";

  auto [rows, cols] = Platform::get_terminal_size();
  std::cout << "  Terminal size: " << rows << "x" << cols << "\n";

  std::cout << "  ✅ Platform tests PASSED\n\n";
}

void test_config() {
  std::cout << "Testing Config management...\n";

  auto &config = Config::instance();

  // Test general settings
  auto shell = config.get_shell_type();
  std::cout << "  Shell type: " << shell << "\n";
  assert(!shell.empty());

  auto history_size = config.get_history_size();
  std::cout << "  History size: " << history_size << "\n";
  assert(history_size > 0);

  bool colors = config.get_colors_enabled();
  std::cout << "  Colors enabled: " << (colors ? "yes" : "no") << "\n";

  // Test AI settings
  auto provider = config.get_ai_provider();
  std::cout << "  AI provider: " << provider << "\n";
  assert(!provider.empty());

  auto model = config.get_ai_model();
  std::cout << "  AI model: " << model << "\n";

  float temp = config.get_ai_temperature();
  std::cout << "  Temperature: " << temp << "\n";
  assert(temp >= 0.0f && temp <= 2.0f);

  // Test llama.cpp settings
  auto model_path = config.get_llamacpp_model_path();
  std::cout << "  Model path: " << model_path << "\n";

  int gpu_layers = config.get_llamacpp_gpu_layers();
  std::cout << "  GPU layers: " << gpu_layers << "\n";

  // Test safety settings
  bool confirm = config.get_confirm_destructive();
  bool explain = config.get_explain_before_run();
  std::cout << "  Confirm destructive: " << (confirm ? "yes" : "no") << "\n";
  std::cout << "  Explain before run: " << (explain ? "yes" : "no") << "\n";
  assert(confirm == true); // Should be true by default for safety

  std::cout << "  ✅ Config tests PASSED\n\n";
}

void test_logging() {
  std::cout << "Testing Logging system...\n";

  // Initialize logger with temp file
  Logger::instance().init("/tmp/si_test.log", Logger::Level::Debug,
                          Logger::Level::Trace);

  // Test logging at various levels
  SI_LOG_TRACE("This is a trace message");
  SI_LOG_DEBUG("This is a debug message");
  SI_LOG_INFO("This is an info message");
  SI_LOG_WARN("This is a warning message");
  SI_LOG_ERROR("This is an error message");

  std::cout << "  Check /tmp/si_test.log for output\n";
  std::cout << "  ✅ Logging tests PASSED\n\n";
}

void test_signals() {
  std::cout << "Testing Signal handling...\n";

  auto &handler = SignalHandler::instance();

  // Initially should not be shutdown
  assert(!handler.shutdown_requested());
  std::cout << "  Initial state: not shutdown\n";

  // Register a handler
  bool callback_called = false;
  handler.register_shutdown_handlers([&callback_called](int sig) {
    callback_called = true;
    std::cout << "  Shutdown callback called with signal " << sig << "\n";
  });

  // Request shutdown
  handler.request_shutdown();
  assert(handler.shutdown_requested());
  std::cout << "  Shutdown requested: confirmed\n";

  std::cout << "  ✅ Signal tests PASSED\n\n";
}

int main() {
  std::cout << "\n";
  std::cout << "========================================\n";
  std::cout << "SI-Core Foundation Component Tests\n";
  std::cout << "========================================\n\n";

  try {
    test_platform();
    test_config();
    test_logging();
    test_signals();

    std::cout << "========================================\n";
    std::cout << "✅ ALL TESTS PASSED!\n";
    std::cout << "========================================\n\n";

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "\n❌ TEST FAILED: " << e.what() << "\n\n";
    return 1;
  }
}
