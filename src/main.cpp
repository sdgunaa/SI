#include "ai/gateway.hpp"
#include "features/interpreter.hpp"
#include "si.hpp"
#include <iostream>

int main(int argc, char **argv) {
  using namespace si;
  using namespace si::foundation;

  try {
    // Initialize logging
    Logger::instance().init((Platform::get_cache_dir() / "si.log").string(),
                            Logger::Level::Info, Logger::Level::Debug);

    SI_LOG_INFO("SI-Core v{} starting...", VERSION);

    // Load configuration
    if (!Config::instance().load_default()) {
      SI_LOG_WARN("Could not load configuration, using defaults");
    }

    // Initialize AI Gateway
    if (!si::ai::AIGateway::instance().initialize()) {
      SI_LOG_WARN("AI Gateway failed to initialize. AI features disabled.");
    }

    // Register signal handlers
    SignalHandler::instance().register_shutdown_handlers([](int sig) {
      SI_LOG_INFO("Received signal {}, shutting down gracefully...", sig);
      SignalHandler::instance().request_shutdown();
    });

    si::features::CommandInterpreter interpreter;

    std::cout << "SI-Core v" << VERSION << " - Shell Intelligence\n";
    std::cout << "Type 'exit' to quit, or '? <request>' for AI command "
                 "generation.\n\n";

    std::string line;
    while (!SignalHandler::instance().shutdown_requested()) {
      if (Platform::is_terminal()) {
        std::cout << "si> " << std::flush;
      }

      if (!std::getline(std::cin, line)) {
        break;
      }

      if (line.empty())
        continue;

      if (line == "exit" || line == "quit") {
        break;
      }

      // AI Command Generation
      if (line.length() > 1 && line[0] == '?') {
        std::string request = line.substr(1);
        // Trim leading space
        size_t first = request.find_first_not_of(' ');
        if (first == std::string::npos)
          continue;
        request = request.substr(first);

        std::cout << "Thinking...\n";
        auto result = interpreter.interpret(request);

        if (result) {
          std::cout << "\nGenerated Command: " << result->command << "\n";
          std::cout << "Explanation:       " << result->explanation << "\n";
          std::cout << "Safety:            "
                    << (result->is_safe ? "Safe" : "Potentially Destructive")
                    << "\n";
          std::cout << "Confidence:        " << (int)(result->confidence * 100)
                    << "%\n";

          std::cout << "\nExecute this command? [y/N] ";
          std::string confirm;
          std::getline(std::cin, confirm);
          if (confirm == "y" || confirm == "Y") {
            std::cout << "Executing...\n";
            int ret = system(result->command.c_str());
            std::cout << "Exit code: " << ret << "\n";
          } else {
            std::cout << "Aborted.\n";
          }
        } else {
          std::cout << "Failed to generate command.\n";
        }
        continue;
      }

      // Built-in commands
      if (line == "version") {
        std::cout << "SI-Core version " << VERSION << "\n";
        continue;
      }

      if (line == "config") {
        std::cout << "Configuration:\n";
        std::cout << "  Shell: " << Config::instance().get_shell_type() << "\n";
        std::cout << "  AI Provider: " << Config::instance().get_ai_provider()
                  << "\n";
        std::cout << "  AI Model: " << Config::instance().get_ai_model()
                  << "\n";
        continue;
      }

      // Fallback execution
      int ret = system(line.c_str());
      if (ret != 0) {
        // TODO: Error analysis
      }
    }

    SI_LOG_INFO("SI-Core shutting down");
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << "\n";
    return 1;
  }
}
