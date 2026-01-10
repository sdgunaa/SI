#pragma once

#include "transport.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace si::mcp {

/**
 * Stdio Transport - Spawns a process and communicates via stdin/stdout
 */
class StdioTransport : public Transport {
public:
  StdioTransport(const std::string &command,
                 const std::vector<std::string> &args = {});
  ~StdioTransport() override;

  bool start() override;
  void close() override;
  bool send(const std::string &message) override;
  void set_message_handler(MessageHandler handler) override;

private:
  void read_loop();

  std::string command_;
  std::vector<std::string> args_;

  int stdin_fd_ = -1;  // Write to child
  int stdout_fd_ = -1; // Read from child
  int stderr_fd_ = -1; // Read from child (for logging)
  int pid_ = -1;

  std::thread read_thread_;
  std::thread err_thread_;
  std::atomic<bool> running_{false};

  MessageHandler handler_;
  std::mutex handler_mutex_;
};

} // namespace si::mcp
