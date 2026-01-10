#pragma once
#include <optional>
#include <string>
#include <vector>

namespace si::features {

struct GitStatus {
  std::string branch;
  bool is_dirty = false;
  int ahead = 0;
  int behind = 0;
  int staged = 0;
  int modified = 0;
  int untracked = 0;
  std::string last_commit_msg;
  bool is_repo = false;
};

class GitContext {
public:
  GitContext(const std::string &path = ".");
  ~GitContext();

  // Check if current directory is in a git repo
  bool in_repo() const;

  // Get current git status
  GitStatus get_status();

  // Get context string for AI prompts
  std::string get_ai_context();

  // Refresh status
  void refresh();

private:
  std::string path_;
  GitStatus status_;
  bool checked_ = false;

  std::string run_git(const std::string &args);
};

} // namespace si::features
