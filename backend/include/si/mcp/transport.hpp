#pragma once

#include <functional>
#include <memory>
#include <string>

namespace si::mcp {

/**
 * Abstract Transport interface for MCP
 */
class Transport {
public:
  virtual ~Transport() = default;

  /**
   * Start the transport (e.g. spawn process, connect socket)
   */
  virtual bool start() = 0;

  /**
   * Close the connection
   */
  virtual void close() = 0;

  /**
   * Send a message string (JSON-RPC)
   */
  virtual bool send(const std::string &message) = 0;

  /**
   * Set callback for received messages
   */
  using MessageHandler = std::function<void(const std::string &)>;
  virtual void set_message_handler(MessageHandler handler) = 0;
};

} // namespace si::mcp
