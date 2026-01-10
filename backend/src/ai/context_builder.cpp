#include "si/ai/context_builder.hpp"
#include "si/foundation/logging.hpp"
#include "si/foundation/platform.hpp"
#include "si/shell/block_manager.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace si::ai {

ContextBuilder &ContextBuilder::instance() {
  static ContextBuilder inst;
  return inst;
}

nlohmann::json ContextBuilder::build_context() {
  nlohmann::json ctx;

  // Current directory
  ctx["cwd"] = current_cwd_;

  // OS info
  ctx["os"] = si::foundation::Platform::get_os_name();
  ctx["shell"] = std::getenv("SHELL") ? std::getenv("SHELL") : "/bin/bash";

  // Git context (if in a git repo)
  std::string git_branch_cmd = "git rev-parse --abbrev-ref HEAD 2>/dev/null";
  FILE *pipe = popen(git_branch_cmd.c_str(), "r");
  if (pipe) {
    char buffer[128];
    std::string branch;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      branch = buffer;
      // Trim newline
      if (!branch.empty() && branch.back() == '\n')
        branch.pop_back();
    }
    pclose(pipe);
    if (!branch.empty()) {
      ctx["git"]["branch"] = branch;
      ctx["git"]["is_repo"] = true;
    } else {
      ctx["git"]["is_repo"] = false;
    }
  }

  // Recent commands (last 5)
  auto &bm = si::shell::BlockManager::instance();
  auto blocks = bm.list_blocks(session_id_);
  nlohmann::json recent = nlohmann::json::array();
  int count = 0;
  for (auto it = blocks.rbegin(); it != blocks.rend() && count < 5;
       ++it, ++count) {
    recent.push_back({{"command", it->command}, {"exit_code", it->exit_code}});
  }
  ctx["recent_commands"] = recent;

  return ctx;
}

std::string ContextBuilder::get_command_generation_prompt() {
  return R"(You are SI, an AI-powered shell assistant. Your role is to translate natural language requests into precise shell commands.

Rules:
1. Output ONLY the command, no explanations.
2. Use common Unix tools when possible.
3. Prefer safe, non-destructive operations.
4. If the request is dangerous, output a safe alternative or refuse.
5. Consider the current directory and git context provided.

Respond with just the command.)";
}

std::string ContextBuilder::get_error_analysis_prompt() {
  return R"(You are SI, an AI assistant for debugging shell errors.

Given a command and its error output, provide:
1. A brief explanation of what went wrong.
2. A suggested fix command if applicable.

Be concise. If suggesting a command, format it in a code block.)";
}

void ContextBuilder::set_cwd(const std::string &cwd) { current_cwd_ = cwd; }

void ContextBuilder::set_session_id(const std::string &session_id) {
  session_id_ = session_id;
}

} // namespace si::ai
