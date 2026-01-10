#pragma once

#include <csignal>
#include <functional>

namespace si::foundation {

/**
 * Signal handling utilities for graceful shutdown
 */
class SignalHandler {
public:
  using SignalCallback = std::function<void(int)>;

  static SignalHandler &instance();

  // Register handler for a specific signal
  void register_handler(int signal, SignalCallback callback);

  // Register common handlers
  void register_shutdown_handlers(SignalCallback callback);

  // Check if shutdown has been requested
  bool shutdown_requested() const;
  void request_shutdown();

private:
  SignalHandler() = default;
  SignalHandler(const SignalHandler &) = delete;
  SignalHandler &operator=(const SignalHandler &) = delete;

  static void signal_handler(int signal);

  bool shutdown_requested_ = false;
};

} // namespace si::foundation
