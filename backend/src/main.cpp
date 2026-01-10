#include "si/ai/gateway.hpp"
#include "si/foundation/config.hpp"
#include "si/foundation/logging.hpp"
#include "si/foundation/platform.hpp"
#include "si/foundation/signals.hpp"
#include "si/rpc/api_bindings.hpp"
#include "si/rpc/server.hpp"
#include "si/session/history.hpp"
#include "si/shell/interactive_shell.hpp"
#include "si/si.hpp"
#include <cstring>
#include <iostream>

using namespace si;
using namespace si::foundation;

int main(int argc, char **argv) {
  try {
    Logger::instance().init((Platform::get_cache_dir() / "si.log").string(),
                            Logger::Level::Info, Logger::Level::Debug);
    SI_LOG_INFO("SI v{} starting...", VERSION);

    Config::instance().load_default();
    si::ai::AIGateway::instance().initialize();
    si::session::HistoryManager::instance().initialize();

    // Register RPC API bindings
    si::rpc::register_api_bindings();

    // Check for --server mode (headless RPC server only)
    bool server_mode = false;
    std::string socket_path = "si.sock";
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--server") {
        server_mode = true;
      } else if (arg == "--socket" && i + 1 < argc) {
        socket_path = argv[++i];
      }
    }

    SignalHandler::instance().register_shutdown_handlers([](int sig) {
      SI_LOG_INFO("Signal {}, shutting down...", sig);
      SignalHandler::instance().request_shutdown();
    });

    if (server_mode) {
      // Headless server mode - start RPC server and wait
      SI_LOG_INFO("Starting in server mode on {}", socket_path);
      std::cout << "SI Backend Server starting on " << socket_path << std::endl;

      if (!si::rpc::RpcServer::instance().start(socket_path)) {
        std::cerr << "Failed to start RPC server\n";
        return 1;
      }

      // Wait for shutdown signal
      while (!SignalHandler::instance().shutdown_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      si::rpc::RpcServer::instance().stop();
    } else {
      // Interactive shell mode
      si::shell::InteractiveShell::instance().run();
    }

    si::session::HistoryManager::instance().shutdown();
    SI_LOG_INFO("SI shutting down");
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Fatal: " << e.what() << "\n";
    return 1;
  }
}
