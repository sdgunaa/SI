#include "si/features/git_context.hpp"
#include "si/shell/executor.hpp"
#include <sstream>

namespace si::features {

GitContext::GitContext(const std::string &path) : path_(path) {}
GitContext::~GitContext() {}

std::string GitContext::run_git(const std::string &args) {
  si::shell::CommandExecutor exec;
  std::string cmd = "git";
  if (!path_.empty() && path_ != ".") {
    cmd += " -C \"" + path_ + "\"";
  }
  cmd += " " + args + " 2>/dev/null";
  auto result = exec.execute(cmd);
  if (result.exit_code != 0)
    return "";

  // Trim trailing newline
  auto &out = result.stdout_output;
  while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
    out.pop_back();
  return out;
}

bool GitContext::in_repo() const { return status_.is_repo; }

void GitContext::refresh() {
  checked_ = true;
  status_ = GitStatus();

  // Check if in repo
  auto toplevel = run_git("rev-parse --show-toplevel");
  if (toplevel.empty()) {
    status_.is_repo = false;
    return;
  }
  status_.is_repo = true;

  // Get branch
  status_.branch = run_git("branch --show-current");
  if (status_.branch.empty())
    status_.branch = run_git("rev-parse --short HEAD");

  // Get status counts
  auto status_output = run_git("status --porcelain");
  std::istringstream iss(status_output);
  std::string line;
  while (std::getline(iss, line)) {
    if (line.length() < 2)
      continue;
    char x = line[0], y = line[1];
    if (x == '?' && y == '?')
      status_.untracked++;
    else if (x != ' ')
      status_.staged++;
    else if (y != ' ')
      status_.modified++;
  }

  status_.is_dirty =
      (status_.staged + status_.modified + status_.untracked) > 0;

  // Get ahead/behind
  auto upstream = run_git("rev-parse --abbrev-ref @{upstream}");
  if (!upstream.empty()) {
    auto ahead_str = run_git("rev-list --count @{upstream}..HEAD");
    auto behind_str = run_git("rev-list --count HEAD..@{upstream}");
    try {
      if (!ahead_str.empty())
        status_.ahead = std::stoi(ahead_str);
      if (!behind_str.empty())
        status_.behind = std::stoi(behind_str);
    } catch (...) {
    }
  }

  // Last commit
  status_.last_commit_msg = run_git("log -1 --format=%s");
}

GitStatus GitContext::get_status() {
  if (!checked_)
    refresh();
  return status_;
}

std::string GitContext::get_ai_context() {
  if (!checked_)
    refresh();
  if (!status_.is_repo)
    return "";

  std::ostringstream ctx;
  ctx << "Git: " << status_.branch;
  if (status_.is_dirty) {
    ctx << " (";
    if (status_.staged > 0)
      ctx << status_.staged << " staged, ";
    if (status_.modified > 0)
      ctx << status_.modified << " modified, ";
    if (status_.untracked > 0)
      ctx << status_.untracked << " untracked";
    ctx << ")";
  }
  if (status_.ahead > 0)
    ctx << " ↑" << status_.ahead;
  if (status_.behind > 0)
    ctx << " ↓" << status_.behind;

  return ctx.str();
}

} // namespace si::features
