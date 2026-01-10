#include "si/foundation/logging.hpp"
#include "si/mcp/client.hpp"
#include "si/mcp/stdio_transport.hpp"
#include <catch2/catch_all.hpp>
#include <filesystem>

using namespace si::mcp;

TEST_CASE("MCP Client Integration", "[mcp]") {
  static bool log_init = false;
  if (!log_init) {
    try {
      si::foundation::Logger::instance().init(
          "test_mcp.log", si::foundation::Logger::Level::Debug,
          si::foundation::Logger::Level::Debug);
    } catch (...) {
    } // Ignore if already initialized
    log_init = true;
  }

  std::string script_path = "../backend/tests/dummy_mcp_server.py";
  if (!std::filesystem::exists(script_path)) {
    // Try other relative paths based on where test is run
    if (std::filesystem::exists("backend/tests/dummy_mcp_server.py"))
      script_path = "backend/tests/dummy_mcp_server.py";
    else if (std::filesystem::exists("tests/dummy_mcp_server.py"))
      script_path = "tests/dummy_mcp_server.py";
    else if (std::filesystem::exists("../tests/dummy_mcp_server.py"))
      script_path = "../tests/dummy_mcp_server.py";
  }

  INFO("Using script: " << script_path);

  auto transport = std::make_unique<StdioTransport>(
      "python3",
      std::vector<std::string>{"-u", // Unbuffered stdio for python is crucial!
                               script_path});

  Client client(std::move(transport));

  REQUIRE(client.initialize());

  SECTION("List Tools") {
    auto tools = client.list_tools();
    REQUIRE(tools.size() == 1);
    REQUIRE(tools[0].name == "echo");
  }

  SECTION("Call Tool") {
    nlohmann::json args = {{"text", "Hello MCP"}};
    auto result = client.call_tool("echo", args);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.size() == 1);
    REQUIRE(result.content[0]["text"] == "Echo: Hello MCP");
  }
}
