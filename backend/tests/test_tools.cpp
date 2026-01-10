#include "si/foundation/logging.hpp"
#include "si/security/permissions.hpp"
#include "si/tools/fs_tool.hpp"
#include <catch2/catch_all.hpp>
#include <filesystem>
#include <fstream>

using namespace si::security;
using namespace si::tools;

TEST_CASE("Security & Tools Integration", "[security]") {
  // Init logger
  static bool log_init = false;
  if (!log_init) {
    try {
      si::foundation::Logger::instance().init(
          "test_security.log", si::foundation::Logger::Level::Debug,
          si::foundation::Logger::Level::Debug);
    } catch (...) {
    }
    log_init = true;
  }

  // Setup dummy file
  std::filesystem::path test_file = "test_data.txt";
  std::ofstream f(test_file);
  f << "secret content";
  f.close();

  FsTool fs_tool;

  // Reset permissions (assuming singleton persists)
  // NOTE: In a real test suite we'd need a way to reset the singleton or inject
  // it. For now, revoke what we might have added.
  PermissionsManager::instance().revoke(PermissionType::READ,
                                        test_file.string());

  SECTION("Default Deny") {
    nlohmann::json args = {{"path", test_file.string()}};
    auto result = fs_tool.execute(args);

    REQUIRE(result.is_error == true);
    REQUIRE(result.content[0]["text"] == "Permission denied");
  }

  SECTION("Grant & Allow") {
    PermissionsManager::instance().grant(PermissionType::READ,
                                         test_file.string());

    nlohmann::json args = {{"path", test_file.string()}};
    auto result = fs_tool.execute(args);

    REQUIRE(result.is_error == false);
    REQUIRE(result.content[0]["text"] == "secret content");
  }

  SECTION("Wildcard Grant") {
    PermissionsManager::instance().grant(PermissionType::READ, "*.txt");

    nlohmann::json args = {{"path", test_file.string()}};
    auto result = fs_tool.execute(args);

    REQUIRE(result.is_error == false);
  }

  std::filesystem::remove(test_file);
}
