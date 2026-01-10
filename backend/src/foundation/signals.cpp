#include "si/foundation/signals.hpp"
#include <signal.h>

namespace si::foundation {

static SignalHandler *g_instance = nullptr;

SignalHandler &SignalHandler::instance() {
  static SignalHandler inst;
  g_instance = &inst;
  return inst;
}

void SignalHandler::register_handler(int signal, SignalCallback callback) {
  // Store callback (simplified - in production use a map)
  ::signal(signal, signal_handler);
}

void SignalHandler::register_shutdown_handlers(SignalCallback callback) {
  register_handler(SIGINT, callback);
  register_handler(SIGTERM, callback);
}

bool SignalHandler::shutdown_requested() const { return shutdown_requested_; }

void SignalHandler::request_shutdown() { shutdown_requested_ = true; }

void SignalHandler::signal_handler(int signal) {
  if (g_instance) {
    g_instance->request_shutdown();
  }
}

} // namespace si::foundation
