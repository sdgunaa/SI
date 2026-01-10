#include "si/foundation/logging.hpp"
#include "si/foundation/platform.hpp"
#include "si/shell/executor.hpp"
#include <catch2/catch_all.hpp>
#include <iostream>

using namespace si::shell;

TEST_CASE("CommandExecutor basics", "[shell]") {
  CommandExecutor executor;

  SECTION("Execute simple command") {
    auto result = executor.execute("echo 'hello world'");
    REQUIRE(result.exit_code == 0);
    REQUIRE(result.stdout_output == "hello world\n");
    REQUIRE(result.success == true);
  }

  SECTION("Execute failing command") {
    auto result = executor.execute("false");
    REQUIRE(result.exit_code != 0);
    REQUIRE(result.success == false);
  }

  SECTION("Capture stderr") {
    auto result = executor.execute("echo 'error info' >&2");
    REQUIRE(result.exit_code == 0);
    REQUIRE(result.stderr_output == "error info\n");
  }

  SECTION("Streaming execution") {
    std::string captured_out;
    int status = executor.execute_stream(
        "echo 'part 1'; echo 'part 2'", ".", "/bin/bash",
        [&](const std::string &s) { captured_out += s; }, nullptr);
    REQUIRE(status == 0);
    // PTY output may have varying whitespace; check for content presence
    REQUIRE_THAT(captured_out, Catch::Matchers::ContainsSubstring("part 1"));
    REQUIRE_THAT(captured_out, Catch::Matchers::ContainsSubstring("part 2"));
  }
}
