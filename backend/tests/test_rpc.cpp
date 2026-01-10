#include "si/foundation/logging.hpp"
#include "si/rpc/server.hpp"
#include <catch2/catch_all.hpp>

using namespace si::rpc;

TEST_CASE("RPC Server Request Handling", "[rpc]") {
  // Init logger
  static bool log_init = false;
  if (!log_init) {
    try {
      si::foundation::Logger::instance().init(
          "test_rpc.log", si::foundation::Logger::Level::Debug,
          si::foundation::Logger::Level::Debug);
    } catch (...) {
    }
    log_init = true;
  }

  auto &rpc = RpcServer::instance();

  // Register a test method
  rpc.register_method("test.echo", [](const nlohmann::json &p) {
    return nlohmann::json{{"echo", p.value("message", "")}};
  });

  SECTION("Valid Request") {
    std::string request =
        R"({"jsonrpc":"2.0","method":"test.echo","params":{"message":"hello"},"id":1})";
    std::string response = rpc.handle_request(request);

    auto j = nlohmann::json::parse(response);
    REQUIRE(j["jsonrpc"] == "2.0");
    REQUIRE(j["id"] == 1);
    REQUIRE(j["result"]["echo"] == "hello");
  }

  SECTION("Method Not Found") {
    std::string request =
        R"({"jsonrpc":"2.0","method":"unknown.method","params":{},"id":2})";
    std::string response = rpc.handle_request(request);

    auto j = nlohmann::json::parse(response);
    REQUIRE(j["error"]["code"] == -32601);
  }

  SECTION("Invalid JSON") {
    std::string request = "this is not json";
    std::string response = rpc.handle_request(request);

    auto j = nlohmann::json::parse(response);
    REQUIRE(j["error"]["code"] == -32700);
  }

  SECTION("Invalid Request (Wrong version)") {
    std::string request = R"({"jsonrpc":"1.0","method":"test.echo","id":3})";
    std::string response = rpc.handle_request(request);

    auto j = nlohmann::json::parse(response);
    REQUIRE(j["error"]["code"] == -32600);
  }
}
