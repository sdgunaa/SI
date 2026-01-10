#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#include "si/foundation/config.hpp"
#include "si/foundation/logging.hpp"
#include "si/foundation/platform.hpp"
#include "si/foundation/signals.hpp"

using namespace si::foundation;

TEST_CASE("Platform utilities work correctly", "[platform]") {
  SECTION("OS detection") {
    auto os = Platform::get_os();
    REQUIRE(os != Platform::OS::Unknown);

    auto os_name = Platform::get_os_name();
    REQUIRE(!os_name.empty());
  }

  SECTION("Home directory") {
    auto home = Platform::get_home_dir();
    REQUIRE(!home.empty());
    REQUIRE(std::filesystem::exists(home));
  }

  SECTION("Config directory") {
    auto config_dir = Platform::get_config_dir();
    REQUIRE(!config_dir.empty());
  }

  SECTION("Environment variables") {
    Platform::set_env("SI_TEST", "value123");
    REQUIRE(Platform::has_env("SI_TEST"));
    REQUIRE(Platform::get_env("SI_TEST") == "value123");
    REQUIRE(Platform::get_env("NONEXISTENT", "default") == "default");
  }

  SECTION("Path expansion") {
    auto expanded = Platform::expand_path("~/test");
    REQUIRE(expanded.string().find('~') == std::string::npos);
  }

  SECTION("Terminal detection") {
    // Just verify these don't crash
    bool is_term = Platform::is_terminal();
    bool has_color = Platform::supports_color();
    auto size = Platform::get_terminal_size();
    REQUIRE(size.first > 0);
    REQUIRE(size.second > 0);
  }
}

TEST_CASE("Config loads with defaults", "[config]") {
  auto &config = Config::instance();

  SECTION("General settings have defaults") {
    auto shell = config.get_shell_type();
    REQUIRE(!shell.empty());

    auto history_size = config.get_history_size();
    REQUIRE(history_size > 0);

    auto colors = config.get_colors_enabled();
    // Just check it returns something
  }

  SECTION("AI settings have defaults") {
    auto provider = config.get_ai_provider();
    REQUIRE(!provider.empty());

    auto model = config.get_ai_model();
    REQUIRE(!model.empty());

    auto temp = config.get_ai_temperature();
    REQUIRE(temp >= 0.0f);
    REQUIRE(temp <= 2.0f);
  }

  SECTION("Safety settings have defaults") {
    auto confirm = config.get_confirm_destructive();
    auto explain = config.get_explain_before_run();
    // Both should be true by default for safety
    REQUIRE(confirm == true);
    REQUIRE(explain == true);
  }
}

TEST_CASE("Logger initializes correctly", "[logging]") {
  SECTION("Singleton instance") {
    auto &logger1 = Logger::instance();
    auto &logger2 = Logger::instance();
    REQUIRE(&logger1 == &logger2);
  }

  SECTION("Logging doesn't crash") {
    SI_LOG_INFO("Test info message");
    SI_LOG_DEBUG("Test debug message");
    SI_LOG_WARN("Test warning message");
    SI_LOG_ERROR("Test error message");
    // If we get here, logging works
    REQUIRE(true);
  }
}

TEST_CASE("SignalHandler works", "[signals]") {
  auto &handler = SignalHandler::instance();

  SECTION("Initially not shutdown requested") {
    REQUIRE(!handler.shutdown_requested());
  }

  SECTION("Can request shutdown") {
    handler.request_shutdown();
    REQUIRE(handler.shutdown_requested());
  }
}
