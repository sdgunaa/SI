#include "si/shell/interactive_shell.hpp"
#include "si/ai/gateway.hpp"
#include "si/features/error_analyzer.hpp"
#include "si/features/file_ops.hpp"
#include "si/features/git_context.hpp"
#include "si/features/interpreter.hpp"
#include "si/foundation/config.hpp"
#include "si/foundation/logging.hpp"
#include "si/foundation/platform.hpp"
#include "si/foundation/signals.hpp"
#include "si/session/history.hpp"
#include "si/shell/executor.hpp"
#include "si/si.hpp"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace si::shell {

namespace {
// Helper function moved from anonymous namespace
bool looks_like_cmd(const std::string &input) {
  if (input.empty())
    return false;
  std::istringstream iss(input);
  std::string first;
  iss >> first;
  if (first.empty())
    return false;
  if (first[0] == '/' || first.substr(0, 2) == "./")
    return true;

  std::string check = "which " + first + " >/dev/null 2>&1";
  if (std::system(check.c_str()) == 0)
    return true;

  static const std::vector<std::string> builtins = {
      "cd",    "echo",  "export", "set",     "unset",    "source", "alias",
      "if",    "for",   "while",  "case",    "function", "return", "exit",
      "pwd",   "type",  "which",  "history", "jobs",     "fg",     "bg",
      "cat",   "ls",    "cp",     "mv",      "rm",       "mkdir",  "rmdir",
      "touch", "chmod", "grep",   "find",    "head",     "tail",   "less",
      "more",  "wc",    "git",    "npm",     "node",     "python", "pip",
      "cargo", "make",  "cmake"};
  for (const auto &b : builtins)
    if (first == b)
      return true;
  return false;
}
} // namespace

InteractiveShell &InteractiveShell::instance() {
  static InteractiveShell instance;
  return instance;
}

InteractiveShell::InteractiveShell() = default;
InteractiveShell::~InteractiveShell() = default;

bool InteractiveShell::looks_like_command(const std::string &input) {
  return looks_like_cmd(input);
}

void InteractiveShell::print_block_header(const std::string &cmd) {
  std::cout << "\033[38;5;239m┌─\033[0m \033[1m" << cmd << "\033[0m\n";
}

void InteractiveShell::print_block_footer(int exit_code, double duration_ms) {
  std::cout << "\033[38;5;239m└─\033[0m ";
  if (exit_code == 0) {
    std::cout << "\033[38;5;40m✓\033[0m";
  } else {
    std::cout << "\033[38;5;196m✗ " << exit_code << "\033[0m";
  }
  std::cout << " \033[38;5;245m" << std::fixed << std::setprecision(0)
            << duration_ms << "ms\033[0m\n\n";
}

void InteractiveShell::run() {
  using namespace si::foundation;

  // Components
  si::features::CommandInterpreter interpreter;
  si::features::ErrorAnalyzer error_analyzer;
  si::features::FileOperations file_ops;
  si::features::GitContext git_ctx;
  si::shell::CommandExecutor executor;
  bool ai_ok = si::ai::AIGateway::instance().is_available();

  // Premium banner
  std::cout << "\n\033[38;5;"
               "39m╭───────────────────────────────────────────╮\033[0m\n";
  std::cout << "\033[38;5;39m│\033[0m  \033[1;97mSI\033[0m \033[38;5;245mv"
            << VERSION
            << "\033[0m  \033[3;38;5;245mShell Intelligence\033[0m           "
               " \033[38;5;39m│\033[0m\n";
  std::cout << "\033[38;5;"
               "39m╰───────────────────────────────────────────╯\033[0m\n";
  std::cout << (ai_ok ? "\033[38;5;40m●\033[0m AI"
                      : "\033[38;5;240m○\033[0m AI offline")
            << "\n\n";

  std::string cwd = std::filesystem::current_path().string();
  std::string line;

  while (!SignalHandler::instance().shutdown_requested()) {
    // Git-aware prompt
    git_ctx.refresh();
    auto gs = git_ctx.get_status();

    if (gs.is_repo) {
      std::cout << "\033[38;5;141m" << gs.branch << "\033[0m";
      if (gs.is_dirty)
        std::cout << "\033[38;5;208m*\033[0m";
      if (gs.ahead > 0)
        std::cout << "\033[38;5;40m↑" << gs.ahead << "\033[0m";
      if (gs.behind > 0)
        std::cout << "\033[38;5;196m↓" << gs.behind << "\033[0m";
      std::cout << " ";
    }
    std::cout << "\033[38;5;39m❯\033[0m " << std::flush;

    if (!std::getline(std::cin, line))
      break;
    if (line.empty())
      continue;
    if (line == "exit" || line == "quit")
      break;

    // Built-ins
    if (line == "version") {
      std::cout << "SI v" << VERSION << "\n";
      continue;
    }
    if (line == "config") {
      std::cout << "\033[1mAI:\033[0m " << Config::instance().get_ai_model()
                << "\n";
      continue;
    }
    if (line == "history") {
      auto h = si::session::HistoryManager::instance().recent(20);
      for (const auto &e : h) {
        std::cout << "\033[38;5;245m" << e.id << "\033[0m " << e.command
                  << "\n";
      }
      continue;
    }
    if (line.substr(0, 9) == "summarize" && line.length() > 10) {
      auto path = line.substr(10);
      std::cout << "\033[3;38;5;245mAnalyzing...\033[0m\n";
      auto s = file_ops.summarize(path);
      if (s) {
        std::cout << "\033[1m" << path << "\033[0m (" << s->language << ", "
                  << s->line_count << " lines)\n";
        std::cout << s->summary << "\n";
      } else {
        std::cout << "\033[38;5;196mCouldn't summarize\033[0m\n";
      }
      continue;
    }
    if (line.substr(0, 7) == "explain" && line.length() > 8) {
      auto path = line.substr(8);
      std::cout << "\033[3;38;5;245mAnalyzing...\033[0m\n";
      auto e = file_ops.explain(path);
      if (e)
        std::cout << *e << "\n";
      else
        std::cout << "\033[38;5;196mCouldn't explain\033[0m\n";
      continue;
    }

    // Command execution with blocks
    if (looks_like_command(line)) {
      print_block_header(line);
      auto start = std::chrono::high_resolution_clock::now();

      auto r = executor.execute(line);

      auto end = std::chrono::high_resolution_clock::now();
      double duration =
          std::chrono::duration<double, std::milli>(end - start).count();

      if (!r.stdout_output.empty()) {
        // Indent output for block style
        std::istringstream oss(r.stdout_output);
        std::string ol;
        while (std::getline(oss, ol)) {
          std::cout << "\033[38;5;239m│\033[0m " << ol << "\n";
        }
      }
      if (!r.stderr_output.empty()) {
        std::istringstream ess(r.stderr_output);
        std::string el;
        while (std::getline(ess, el)) {
          std::cout << "\033[38;5;239m│\033[0m \033[38;5;196m" << el
                    << "\033[0m\n";
        }
      }

      print_block_footer(r.exit_code, duration);
      si::session::HistoryManager::instance().add(line, cwd, r.exit_code);

      // Auto-fix suggestions
      if (r.exit_code != 0 && ai_ok && !r.stderr_output.empty()) {
        auto fix = error_analyzer.analyze(line, r.stderr_output, r.exit_code);
        if (fix) {
          std::cout << "\033[38;5;39m💡 Suggest:\033[0m " << fix->fixed_command
                    << "\n";
          std::cout << "   \033[38;5;245m" << fix->explanation << "\033[0m\n";
          std::cout << "   \033[1mApply?\033[0m [y/N] ";
          std::string c;
          std::getline(std::cin, c);
          if (c == "y" || c == "Y") {
            print_block_header(fix->fixed_command);
            auto start2 = std::chrono::high_resolution_clock::now();
            auto fr = executor.execute(fix->fixed_command);
            auto end2 = std::chrono::high_resolution_clock::now();
            double dur2 =
                std::chrono::duration<double, std::milli>(end2 - start2)
                    .count();
            if (!fr.stdout_output.empty()) {
              std::istringstream oss(fr.stdout_output);
              std::string ol;
              while (std::getline(oss, ol))
                std::cout << "\033[38;5;239m│\033[0m " << ol << "\n";
            }
            print_block_footer(fr.exit_code, dur2);
            si::session::HistoryManager::instance().add(fix->fixed_command, cwd,
                                                        fr.exit_code);
          }
        }
      }
    } else if (ai_ok) {
      // AI interpretation with git context
      std::cout << "\033[3;38;5;245mThinking...\033[0m\n";
      auto result = interpreter.interpret(line);
      if (result) {
        std::cout << "\n\033[38;5;39m╭─ AI Generated\033[0m\n";
        std::cout << "\033[38;5;39m│\033[0m \033[1m" << result->command
                  << "\033[0m\n";
        std::cout << "\033[38;5;39m│\033[0m \033[38;5;245m"
                  << result->explanation << "\033[0m\n";
        std::cout << "\033[38;5;39m│\033[0m "
                  << (result->is_safe ? "\033[38;5;40m●\033[0m Safe"
                                      : "\033[38;5;196m●\033[0m Risky");
        std::cout << "  \033[38;5;245m" << (int)(result->confidence * 100)
                  << "%\033[0m\n";
        std::cout << "\033[38;5;39m╰─\033[0m \033[1mRun?\033[0m [y/N] ";
        std::string c;
        std::getline(std::cin, c);
        if (c == "y" || c == "Y") {
          print_block_header(result->command);
          auto start = std::chrono::high_resolution_clock::now();
          auto r = executor.execute(result->command);
          auto end = std::chrono::high_resolution_clock::now();
          double dur =
              std::chrono::duration<double, std::milli>(end - start).count();
          if (!r.stdout_output.empty()) {
            std::istringstream oss(r.stdout_output);
            std::string ol;
            while (std::getline(oss, ol))
              std::cout << "\033[38;5;239m│\033[0m " << ol << "\n";
          }
          print_block_footer(r.exit_code, dur);
          si::session::HistoryManager::instance().add(result->command, cwd,
                                                      r.exit_code);
        }
      } else {
        std::cout << "\033[38;5;196mCouldn't interpret\033[0m\n";
      }
    } else {
      auto r = executor.execute(line);
      if (!r.stdout_output.empty())
        std::cout << r.stdout_output;
      if (!r.stderr_output.empty())
        std::cerr << r.stderr_output;
      si::session::HistoryManager::instance().add(line, cwd, r.exit_code);
    }
  }
}

} // namespace si::shell
