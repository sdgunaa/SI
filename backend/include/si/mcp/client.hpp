#pragma once

#include "transport.hpp"
#include "types.hpp"
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace si::mcp {

/**
 * MCP Client - Implements the protocol logic
 */
class Client {
public:
  explicit Client(std::unique_ptr<Transport> transport);
  ~Client();

  /**
   * Connect and handshake
   */
  bool initialize();

  /**
   * List available tools on the server
   */
  std::vector<Tool> list_tools();

  /**
   * Call a specific tool
   */
  ToolResult call_tool(const std::string &name,
                       const nlohmann::json &arguments);

private:
  // Handles incoming JSON-RPC messages
  void handle_message(const std::string &msg);

  // Sends a request and waits for response (blocking)
  nlohmann::json call(const std::string &method, const nlohmann::json &params);

  std::unique_ptr<Transport> transport_;
  int next_id_ = 1;

  struct PendingRequest {
    std::promise<nlohmann::json> promise;
  };

  std::map<int, std::unique_ptr<PendingRequest>> pending_requests_;
  std::mutex mutex_;
  bool initialized_ = false;
};

} // namespace si::mcp
